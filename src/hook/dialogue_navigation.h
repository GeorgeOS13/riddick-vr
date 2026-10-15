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

} // namespace RiddickVR
