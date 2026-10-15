#pragma once

#include <cmath>

namespace RiddickVR {

enum class DialogueDirection : int {
    Neutral = 0,
    Up = 1,
    Down = 2,
    Left = 3,
    Right = 4
};

inline DialogueDirection ChooseDialogueDirection(float leftX, float leftY,
        float rightX, float rightY, float deadzone) {
    const float leftMagnitudeSq = leftX * leftX + leftY * leftY;
    const float rightMagnitudeSq = rightX * rightX + rightY * rightY;
    const float x = (rightMagnitudeSq >= leftMagnitudeSq) ? rightX : leftX;
    const float y = (rightMagnitudeSq >= leftMagnitudeSq) ? rightY : leftY;
    const float magnitude = sqrtf(x * x + y * y);
    if (magnitude < deadzone) return DialogueDirection::Neutral;
    if (fabsf(x) > fabsf(y)) return x > 0.0f ? DialogueDirection::Right : DialogueDirection::Left;
    return y > 0.0f ? DialogueDirection::Up : DialogueDirection::Down;
}

// GH #53 (3.3): stateful selector layered on ChooseDialogueDirection.
// - Edge-triggered: a held direction taps once and never repeats mid-hold.
// - Axis hysteresis: once a direction is engaged, switching to the other axis
//   (left/right <-> up/down) requires the new axis to dominate by a margin, so
//   a stick parked on a diagonal boundary cannot flicker between two taps.
// Recentring is the caller's responsibility: reset the state when the radial
// magnitude falls inside the release deadzone, which re-arms the same
// direction for the next deliberate push.
struct DialogueNavigationState {
    DialogueDirection active = DialogueDirection::Neutral;
};

// Minimum dominance (candidate-axis magnitude / engaged-axis magnitude) needed
// to switch axes while a direction is already engaged. 1.0 would flip at the
// exact diagonal; 1.2 keeps a guard band against thumbstick jitter.
constexpr float kDialogueAxisSwitchMargin = 1.20f;

inline void ResetDialogueNavigation(DialogueNavigationState& state) {
    state.active = DialogueDirection::Neutral;
}

// Returns the direction to fire this frame (Neutral = nothing new). The caller
// keeps the returned value edge-triggered by holding the state between frames.
inline DialogueDirection UpdateDialogueNavigation(DialogueNavigationState& state,
        float leftX, float leftY, float rightX, float rightY, float deadzone) {
    const DialogueDirection candidate =
        ChooseDialogueDirection(leftX, leftY, rightX, rightY, deadzone);
    if (candidate == DialogueDirection::Neutral || candidate == state.active)
        return DialogueDirection::Neutral; // released, or same direction still held

    // Mirror ChooseDialogueDirection's stronger-stick selection to read the axis
    // components that produced the candidate.
    const float leftMagnitudeSq = leftX * leftX + leftY * leftY;
    const float rightMagnitudeSq = rightX * rightX + rightY * rightY;
    const float x = (rightMagnitudeSq >= leftMagnitudeSq) ? rightX : leftX;
    const float y = (rightMagnitudeSq >= leftMagnitudeSq) ? rightY : leftY;

    const bool candidateHorizontal = (candidate == DialogueDirection::Left ||
                                      candidate == DialogueDirection::Right);
    const bool engagedHorizontal = (state.active == DialogueDirection::Left ||
                                    state.active == DialogueDirection::Right);
    if (state.active != DialogueDirection::Neutral && candidateHorizontal != engagedHorizontal) {
        const float candidateMagnitude = candidateHorizontal ? fabsf(x) : fabsf(y);
        const float engagedMagnitude   = candidateHorizontal ? fabsf(y) : fabsf(x);
        if (candidateMagnitude < engagedMagnitude * kDialogueAxisSwitchMargin)
            return DialogueDirection::Neutral; // hold the engaged direction (no flicker)
    }

    state.active = candidate;
    return candidate;
}

} // namespace RiddickVR
