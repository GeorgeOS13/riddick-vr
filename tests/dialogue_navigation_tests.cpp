#include "dialogue_navigation.h"
#include <cstdio>

using RiddickVR::ChooseDialogueDirection;
using RiddickVR::DialogueDirection;

static int Fail(const char* message) {
    std::fprintf(stderr, "dialogue navigation test failed: %s\n", message);
    return 1;
}

int main() {
    constexpr float deadzone = 0.50f;
    if (ChooseDialogueDirection(0, 0.49f, 0, 0, deadzone) != DialogueDirection::Neutral)
        return Fail("sub-deadzone input selected a direction");
    if (ChooseDialogueDirection(0, 0.8f, 0, 0, deadzone) != DialogueDirection::Up ||
        ChooseDialogueDirection(0, -0.8f, 0, 0, deadzone) != DialogueDirection::Down ||
        ChooseDialogueDirection(-0.8f, 0, 0, 0, deadzone) != DialogueDirection::Left ||
        ChooseDialogueDirection(0.8f, 0, 0, 0, deadzone) != DialogueDirection::Right)
        return Fail("cardinal direction mapping incorrect");
    if (ChooseDialogueDirection(0.8f, 0.7f, 0, 0, deadzone) != DialogueDirection::Right ||
        ChooseDialogueDirection(0.5f, 0.5f, 0, 0, deadzone) != DialogueDirection::Up)
        return Fail("diagonal tie-break or dominant-axis mapping incorrect");
    if (ChooseDialogueDirection(0, 0, -0.7f, 0, deadzone) != DialogueDirection::Left)
        return Fail("stronger right-stick direction was not selected");
    std::puts("dialogue navigation tests passed");
    return 0;
}
