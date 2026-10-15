#pragma once

// Track C #10: minimal riddick_vr.ini loader (no external deps).
// Read once at CameraHook init; all fields have safe defaults so a missing
// or malformed INI never breaks the build or runtime behavior.

#include <windows.h>

namespace RiddickVR {

struct VRConfig {
    float snapAngleDeg = 45.0f;      // snap-turn step size
    float smoothTurnSpeed = 20.0f;   // smooth-turn curve gain
    float moveDeadzone = 0.22f;      // left-stick radial deadzone
    float turnDeadzone = 0.20f;      // right-stick smooth-turn deadzone
    int comfortEnabled = 1;          // #9 comfort vignette proxy toggle
    float comfortStrength = 1.0f;    // vignette proxy gain multiplier
    float comfortMaxDim = 0.45f;     // GH #9: peripheral dimming ceiling (0=off, max alpha)
    int seated = 0;                  // #11 seated mode
    float seatedEyeOffset = 0.15f;   // #11 seated eye-height compensation
    float standingEyeHeight = 0.0f;  // optional baseline override (0 = auto)
    float worldScale = 1.0f;         // #13 world scale multiplier
    int southpaw = 0;                // #14 left-dominant hand mapping
    int gestureDebounceMs = 180;     // #15 per-action debounce (clamped 120-250)
};

extern VRConfig g_vrConfig;

void LoadVRConfig();

inline float GetWorldScale() {
    return (g_vrConfig.worldScale > 0.001f) ? g_vrConfig.worldScale : 1.0f;
}

} // namespace RiddickVR
