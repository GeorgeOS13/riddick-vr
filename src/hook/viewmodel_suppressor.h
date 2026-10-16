#pragma once

// GH #63 / GH #66 (approach 2): per-draw suppression of the engine's own
// first-person viewmodel (arms/hands).
//
// Approach 1 (forcing the RPG item's `noviewmodel` bit) was falsified in
// ADR-0438: the flag is an engine *output* (set/cleared every frame by
// `FUN_100e6aa0`) and an exhaustive raw-byte scan of every shipped x86 module
// finds no direct `[item+0x14c] & 2` reader. Approach 2 suppresses at the draw
// level instead: the hook forwards the engine's GL draw calls and this header
// decides whether a given draw belongs to the first-person viewmodel pass.
//
// Evidence for the signature candidates (docs/rea/reference/RndrGL_GL_Path.txt):
//   * RndrGL.dll is the *only* shipped module that imports OpenGL. Its geometry
//     path is `glDrawElements` (vertex arrays) and `glCallList` (display lists);
//     it also imports `glBegin`/`glEnd` but no `glDrawArrays`.
//   * It imports `glClear`, `glLoadMatrixf`, `glFrustum`, `glViewport`, `glScissor`
//     -- all already intercepted -- so a mid-frame *depth-only* clear
//     (GL_DEPTH_BUFFER_BIT with no colour bit) is observable through the entry
//     points the hook already sees.
// The classic first-person-era trick is exactly that: draw the world, clear
// depth, then draw the viewmodel so it never intersects walls. So the first
// signature is "the window opened by a depth-only clear, closed by the next
// projection load or the frame swap".
//
// This is a PROVISIONAL heuristic: it is OFF by default (SuppressEngineViewmodel
// = 0) and must be confirmed on a headset (one run with the counters logged)
// before it is trusted. When disabled, ShouldSuppressDraw() is a pure `false`
// and the draw hooks forward verbatim -- zero behaviour change.
//
// Pure, dependency-free state machine: it uses no Windows/GL headers so it
// compiles on the x64 host and is unit-tested headlessly (no game launch), like
// combat_guard.h / combat_strike.h.

namespace RiddickVR {

// Which observed event opens the suppression window.
enum class ViewmodelWindowOpener {
    None,             // never open: the suppressor is inert even when enabled
    DepthOnlyClear,   // GL_DEPTH_BUFFER_BIT clear with no colour bit
};

struct ViewmodelSuppressionConfig {
    // Master switch. false => ShouldSuppressDraw() is always false.
    bool enabled = false;
    // Window opener (see the header comment). Default is the classic
    // depth-only-clear boundary; PROVISIONAL, select on a headset.
    ViewmodelWindowOpener opener = ViewmodelWindowOpener::DepthOnlyClear;
};

// The suppressor tracks one frame at a time. Frame boundaries are the
// full (colour) clear and the buffer swap; a depth-only clear may open the
// suppression window mid-frame, and the next projection load or swap closes it.
class ViewmodelSuppressor {
public:
    void Configure(const ViewmodelSuppressionConfig& cfg) { config_ = cfg; }

    const ViewmodelSuppressionConfig& Config() const { return config_; }
    bool Enabled() const { return config_.enabled; }
    bool WindowOpen() const { return windowOpen_; }

    // Frame start: a full colour clear. Closes any open window and re-bases the
    // per-frame counters.
    void NoteColorClear() {
        windowOpen_ = false;
        LatchFrameCounters();
    }

    // Mid-frame depth-only clear. Opens the window when that opener is selected.
    void NoteDepthOnlyClear() {
        if (config_.enabled && config_.opener == ViewmodelWindowOpener::DepthOnlyClear) {
            windowOpen_ = true;
        }
    }

    // Any projection (re)load ends the first-person pass: the world resumes.
    void NoteProjectionLoad() { windowOpen_ = false; }

    // Buffer swap: end of frame. Close the window and publish the frame tallies.
    void NoteSwap() {
        windowOpen_ = false;
        LatchFrameCounters();
    }

    // Called once per intercepted engine draw call. Returns true when this draw
    // should be skipped. Counters are only maintained while enabled so the
    // disabled path stays a single predictable branch.
    bool ShouldSuppressDraw() {
        if (!config_.enabled) return false;
        ++frameDraws_;
        if (windowOpen_) {
            ++frameSuppressed_;
            ++totalSuppressed_;
            return true;
        }
        return false;
    }

    // Per-frame tallies (draws seen / draws suppressed while enabled).
    unsigned FrameDraws() const { return frameDraws_; }
    unsigned FrameSuppressed() const { return frameSuppressed_; }
    // Tallies from the most recently completed frame.
    unsigned LastFrameDraws() const { return lastFrameDraws_; }
    unsigned LastFrameSuppressed() const { return lastFrameSuppressed_; }
    // Cumulative suppressed draws since Configure.
    unsigned long long TotalSuppressed() const { return totalSuppressed_; }

private:
    void LatchFrameCounters() {
        lastFrameDraws_ = frameDraws_;
        lastFrameSuppressed_ = frameSuppressed_;
        frameDraws_ = 0;
        frameSuppressed_ = 0;
    }

    ViewmodelSuppressionConfig config_{};
    bool windowOpen_ = false;
    unsigned frameDraws_ = 0;
    unsigned frameSuppressed_ = 0;
    unsigned lastFrameDraws_ = 0;
    unsigned lastFrameSuppressed_ = 0;
    unsigned long long totalSuppressed_ = 0;
};

} // namespace RiddickVR
