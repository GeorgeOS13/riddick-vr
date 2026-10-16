// GH #65: unit tests for the controller-swing strike classifier
// (combat_strike.h).
//
// The classifier decides whether a frame's controller velocities are a jab
// (forward), a hook, an uppercut, or a dual cross-slash, and produces the
// combined `strike` gate. These tests pin:
//   - the e858809 first-pass thresholds (the INI defaults),
//   - the ADR-0403 guarantee that *raising the hands into guard* is never a
//     strike (the bug GH #65 first pass had to avoid re-introducing), and
//   - that the thresholds are data-driven (tuning the struct changes the
//     outcome), which is what makes them INI-tunable on a headset.
#include "combat_strike.h"

#include <cstdio>

using RiddickVR::ClassifyStrike;
using RiddickVR::StrikeIntent;
using RiddickVR::StrikeThresholds;
using RiddickVR::StrikeVelocity;

static int g_failures = 0;

static void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "combat strike test failed: %s\n", message);
        ++g_failures;
    }
}

int main() {
    const StrikeThresholds t;  // defaults (e858809 first-pass numbers)

    // Defaults are the documented thresholds.
    Check(t.forwardVz == -1.20f && t.forwardMaxZ == -0.10f, "default forward thresholds drifted");
    Check(t.hookVx == 1.35f && t.hookVz == -0.15f, "default hook thresholds drifted");
    Check(t.uppercutVy == 1.35f && t.uppercutMaxY == -0.10f && t.uppercutVz == -0.20f,
          "default uppercut thresholds drifted");
    Check(t.crossVx == 1.10f, "default cross-slash threshold drifted");

    const StrikeVelocity still{0.0f, 0.0f, 0.0f};

    // A straight jab: fast forward velocity, hand ahead of the HMD plane.
    StrikeIntent jab = ClassifyStrike(t, still, StrikeVelocity{0.0f, 0.0f, -1.30f},
                                      0.0f, -0.30f, 0.0f, -0.25f);
    Check(jab.forwardPunch && jab.strike, "a forward jab must register a strike");
    Check(!jab.hookPunch && !jab.uppercutPunch && !jab.dualCrossSlash,
          "a straight jab must not read as hook/uppercut/cross");

    // Whitespace/velocity thresholds are strict (`<`), exactly like the
    // original inline classifier: a swing at the threshold does not fire.
    Check(!ClassifyStrike(t, still, StrikeVelocity{0.0f, 0.0f, -1.20f}, 0.0f, -0.30f, 0.0f, -0.30f).strike,
          "forward Vz exactly at the threshold must not fire (strict)");
    Check(!ClassifyStrike(t, still, StrikeVelocity{0.0f, 0.0f, -1.30f}, 0.0f, -0.05f, 0.0f, -0.05f).strike,
          "a fast swing with the hand inside/behind the view plane must not fire");

    // The left hand drives strikes too (symmetry).
    Check(ClassifyStrike(t, StrikeVelocity{0.0f, 0.0f, -1.30f}, still, 0.0f, -0.25f, 0.0f, -0.30f).forwardPunch,
          "a left-hand jab must register");

    // A hook: fast lateral swing that also travels forward.
    StrikeIntent hook = ClassifyStrike(t, still, StrikeVelocity{1.50f, 0.0f, -0.20f},
                                       0.0f, -0.30f, 0.0f, -0.20f);
    Check(hook.hookPunch && hook.strike, "a lateral hook with forward travel must register");
    Check(ClassifyStrike(t, still, StrikeVelocity{-1.50f, 0.0f, -0.20f}, 0.0f, -0.30f, 0.0f, -0.20f).hookPunch,
          "a sign-agnostic lateral hook must register");
    Check(!ClassifyStrike(t, still, StrikeVelocity{1.50f, 0.0f, 0.0f}, 0.0f, -0.30f, 0.0f, -0.20f).hookPunch,
          "a pure sideways sweep with no forward travel must not be a hook");
    Check(!ClassifyStrike(t, still, StrikeVelocity{1.35f, 0.0f, -0.20f}, 0.0f, -0.30f, 0.0f, -0.20f).strike,
          "hook |Vx| exactly at the threshold must not fire (strict)");

    // An uppercut: fast upward drive from below the view, with forward travel.
    StrikeIntent uppercut = ClassifyStrike(t, still, StrikeVelocity{0.0f, 1.50f, -0.25f},
                                           0.0f, -0.30f, -0.20f, -0.25f);
    Check(uppercut.uppercutPunch && uppercut.strike, "an upward drive from below the view must register");
    Check(!ClassifyStrike(t, still, StrikeVelocity{0.0f, 1.50f, -0.25f}, 0.0f, -0.30f, 0.10f, -0.25f).uppercutPunch,
          "an uppercut must start below the view (y < uppercutMaxY)");
    Check(!ClassifyStrike(t, still, StrikeVelocity{0.0f, 1.50f, 0.0f}, 0.0f, -0.30f, -0.20f, -0.25f).uppercutPunch,
          "an uppercut must travel forward (Vz < uppercutVz)");

    // Dual cross-slash: opposing horizontal velocities.
    Check(ClassifyStrike(t, StrikeVelocity{1.50f, 0.0f, 0.0f}, StrikeVelocity{-1.50f, 0.0f, 0.0f},
                         0.0f, -0.30f, 0.0f, -0.30f).dualCrossSlash,
          "opposing horizontal sweeps must register a dual cross-slash");
    Check(!ClassifyStrike(t, StrikeVelocity{1.50f, 0.0f, 0.0f}, StrikeVelocity{1.50f, 0.0f, 0.0f},
                          0.0f, -0.30f, 0.0f, -0.30f).dualCrossSlash,
          "same-direction sweeps must not be a dual cross-slash");
    Check(!ClassifyStrike(t, StrikeVelocity{1.05f, 0.0f, 0.0f}, StrikeVelocity{-1.05f, 0.0f, 0.0f},
                          0.0f, -0.30f, 0.0f, -0.30f).strike,
          "a slow opposing sweep must not fire (crossVx strict)");

    // The combined gate is the OR of the four classifiers.
    Check(ClassifyStrike(t, StrikeVelocity{1.50f, 0.0f, 0.0f}, StrikeVelocity{-1.50f, 0.0f, 0.0f},
                         0.0f, -0.30f, 0.0f, -0.30f).strike,
          "the combined strike gate must include the dual cross-slash");

    // GH #65 / ADR-0403 regression: *raising* the hands into guard must never
    // read as a strike. A guard raise is upward with ~zero forward velocity.
    Check(!ClassifyStrike(t, StrikeVelocity{0.0f, 1.20f, 0.0f}, StrikeVelocity{0.0f, 1.20f, 0.0f},
                          0.0f, -0.30f, 0.0f, -0.30f).strike,
          "raising both hands into guard must never be a strike");
    // Even a fast raise is not an uppercut when the hands are not below the view.
    Check(!ClassifyStrike(t, StrikeVelocity{0.0f, 1.80f, 0.0f}, StrikeVelocity{0.0f, 1.80f, 0.0f},
                          0.05f, -0.30f, 0.05f, -0.30f).strike,
          "a fast guard raise above the view must never be an uppercut");
    // Idle hands never fire.
    Check(!ClassifyStrike(t, still, still, 0.0f, -0.30f, 0.0f, -0.30f).strike,
          "idle hands must never fire a strike");

    // Tuned thresholds are honoured (this is the INI path).
    StrikeThresholds tuned = t;
    tuned.forwardVz = -2.00f;  // desensitised: a -1.30 jab no longer fires
    Check(!ClassifyStrike(tuned, still, StrikeVelocity{0.0f, 0.0f, -1.30f}, 0.0f, -0.30f, 0.0f, -0.30f).forwardPunch,
          "a tightened forward threshold must suppress a weaker jab");
    tuned.forwardVz = -0.50f;  // oversensitive: a gentle push now fires
    Check(ClassifyStrike(tuned, still, StrikeVelocity{0.0f, 0.0f, -0.60f}, 0.0f, -0.30f, 0.0f, -0.30f).forwardPunch,
          "a loosened forward threshold must fire on a gentler push");
    tuned = t;
    tuned.crossVx = 2.00f;
    Check(!ClassifyStrike(tuned, StrikeVelocity{1.50f, 0.0f, 0.0f}, StrikeVelocity{-1.50f, 0.0f, 0.0f},
                          0.0f, -0.30f, 0.0f, -0.30f).dualCrossSlash,
          "a raised cross threshold must suppress a weaker cross-slash");

    if (g_failures == 0) {
        std::puts("combat strike tests passed");
    }
    return g_failures == 0 ? 0 : 1;
}
