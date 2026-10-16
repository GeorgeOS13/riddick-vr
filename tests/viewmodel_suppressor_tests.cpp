// GH #66 (approach 2): unit tests for the per-draw viewmodel suppression
// window (viewmodel_suppressor.h).
//
// These tests pin the *inert-by-default* guarantee (a disabled suppressor never
// skips a draw) and the window state machine the draw hooks rely on:
//   - a depth-only clear opens the window,
//   - a projection load or the frame swap closes it,
//   - only draws inside the window are suppressed,
//   - a full colour clear re-bases the per-frame counters.
#include "viewmodel_suppressor.h"

#include <cstdio>

using RiddickVR::ViewmodelSuppressionConfig;
using RiddickVR::ViewmodelSuppressor;
using RiddickVR::ViewmodelWindowOpener;

static int g_failures = 0;

static void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "viewmodel suppressor test failed: %s\n", message);
        ++g_failures;
    }
}

int main() {
    // Disabled: inert even inside a depth-only clear window.
    {
        ViewmodelSuppressor s;
        s.Configure(ViewmodelSuppressionConfig{}); // default: enabled == false
        s.NoteColorClear();
        s.NoteDepthOnlyClear();
        for (int i = 0; i < 5; ++i) {
            Check(!s.ShouldSuppressDraw(), "a disabled suppressor must never skip a draw");
        }
        Check(!s.WindowOpen(), "a disabled suppressor must not open a window");
        Check(s.TotalSuppressed() == 0, "a disabled suppressor must not count suppressions");
    }

    // Enabled but opener == None: still inert (no window ever opens).
    {
        ViewmodelSuppressor s;
        ViewmodelSuppressionConfig cfg;
        cfg.enabled = true;
        cfg.opener = ViewmodelWindowOpener::None;
        s.Configure(cfg);
        s.NoteColorClear();
        s.NoteDepthOnlyClear();
        Check(!s.WindowOpen(), "opener None must not open a window");
        Check(!s.ShouldSuppressDraw(), "opener None must never suppress");
        Check(s.FrameDraws() == 1 && s.FrameSuppressed() == 0,
              "opener None must count the draw but not suppress it");
    }

    // Enabled + depth-clear opener: the world (before the depth-only clear) is
    // never suppressed; the viewmodel window is.
    {
        ViewmodelSuppressor s;
        ViewmodelSuppressionConfig cfg;
        cfg.enabled = true;
        cfg.opener = ViewmodelWindowOpener::DepthOnlyClear;
        s.Configure(cfg);

        s.NoteColorClear();                 // frame start: world pass
        Check(!s.ShouldSuppressDraw(), "world geometry before the depth clear must draw");
        Check(!s.ShouldSuppressDraw(), "world geometry before the depth clear must draw");
        Check(!s.WindowOpen(), "the window must be closed before a depth-only clear");

        s.NoteDepthOnlyClear();             // first-person pass begins
        Check(s.WindowOpen(), "a depth-only clear must open the window");
        Check(s.ShouldSuppressDraw(), "the viewmodel draw must be suppressed");
        Check(s.ShouldSuppressDraw(), "every viewmodel draw must be suppressed");

        s.NoteProjectionLoad();             // world (e.g. next pass) resumes
        Check(!s.WindowOpen(), "a projection load must close the window");
        Check(!s.ShouldSuppressDraw(), "draws after the window closes must draw");

        s.NoteSwap();                       // end of frame
        Check(s.LastFrameDraws() == 5, "the frame must count every enabled draw");
        Check(s.LastFrameSuppressed() == 2, "the frame must count only windowed draws");
        Check(s.TotalSuppressed() == 2, "cumulative suppressions must accumulate");
        Check(s.FrameDraws() == 0 && s.FrameSuppressed() == 0,
              "a swap must re-base the per-frame counters");
    }

    // A swap closes an open window (frame ends before a projection load).
    {
        ViewmodelSuppressor s;
        ViewmodelSuppressionConfig cfg;
        cfg.enabled = true;
        s.Configure(cfg);
        s.NoteColorClear();
        s.NoteDepthOnlyClear();
        Check(s.ShouldSuppressDraw(), "a windowed draw must be suppressed");
        s.NoteSwap();
        Check(!s.WindowOpen(), "a swap must close an open window");
        Check(!s.ShouldSuppressDraw(), "draws after a swap must draw");
    }

    // A full colour clear also closes an open window and re-bases the counters
    // (a depth-only clear that is never followed by a projection load must not
    // leak into the next frame).
    {
        ViewmodelSuppressor s;
        ViewmodelSuppressionConfig cfg;
        cfg.enabled = true;
        s.Configure(cfg);
        s.NoteColorClear();
        s.NoteDepthOnlyClear();
        Check(s.ShouldSuppressDraw(), "a windowed draw must be suppressed");
        s.NoteColorClear();                 // next frame begins
        Check(!s.WindowOpen(), "a colour clear must close the window");
        Check(!s.ShouldSuppressDraw(), "the next frame's world must draw");
        Check(s.LastFrameDraws() == 1 && s.LastFrameSuppressed() == 1,
              "a colour clear must latch the previous frame's tallies");
    }

    // Idempotent opens: repeated depth-only clears do not double-count or
    // change the decision, and re-configuring off stops suppression at once.
    {
        ViewmodelSuppressor s;
        ViewmodelSuppressionConfig cfg;
        cfg.enabled = true;
        s.Configure(cfg);
        s.NoteColorClear();
        s.NoteDepthOnlyClear();
        s.NoteDepthOnlyClear();
        Check(s.ShouldSuppressDraw() && s.FrameSuppressed() == 1,
              "repeated depth-only clears must not double-count");
        ViewmodelSuppressionConfig off = cfg;
        off.enabled = false;
        s.Configure(off);
        Check(!s.ShouldSuppressDraw(), "re-configuring the suppressor off must stop suppression");
    }

    if (g_failures == 0) {
        std::puts("viewmodel suppressor tests passed");
    }
    return g_failures == 0 ? 0 : 1;
}
