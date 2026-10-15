#include "dialogue_navigation.h"
#include <cstdio>

using RiddickVR::ChooseDialogueDirection;
using RiddickVR::DialogueDirection;
using RiddickVR::DialogueNavigationState;
using RiddickVR::ResetDialogueNavigation;
using RiddickVR::UpdateDialogueNavigation;

static int Fail(const char* message) {
    std::fprintf(stderr, "dialogue navigation test failed: %s\n", message);
    return 1;
}

int main() {
    constexpr float deadzone = 0.50f;

    // --- Stateless direction mapping (ChooseDialogueDirection) -------------
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

    // --- GH #53 (3.3): edge-trigger + axis hysteresis ---------------------
    {
        DialogueNavigationState nav{};
        if (UpdateDialogueNavigation(nav, 0, 0.8f, 0, 0, deadzone) != DialogueDirection::Up)
            return Fail("first Up deflection did not fire");
        for (int i = 0; i < 30; ++i)
            if (UpdateDialogueNavigation(nav, 0, 0.8f, 0, 0, deadzone) != DialogueDirection::Neutral)
                return Fail("held direction repeated while held");

        // Stick parked on the Up/Right diagonal must not flicker either way.
        if (UpdateDialogueNavigation(nav, 0.62f, 0.60f, 0, 0, deadzone) != DialogueDirection::Neutral)
            return Fail("diagonal wobble toward Right flickered a new direction");
        if (UpdateDialogueNavigation(nav, 0.60f, 0.62f, 0, 0, deadzone) != DialogueDirection::Neutral)
            return Fail("diagonal wobble back toward Up flickered a new direction");

        // A clearly dominant Right axis still legitimately switches.
        if (UpdateDialogueNavigation(nav, 0.90f, 0.30f, 0, 0, deadzone) != DialogueDirection::Right)
            return Fail("dominant Right axis did not switch direction");
        if (UpdateDialogueNavigation(nav, 0.90f, 0.30f, 0, 0, deadzone) != DialogueDirection::Neutral)
            return Fail("held Right repeated while held");

        // Recentring re-arms the same direction for a fresh tap.
        ResetDialogueNavigation(nav);
        if (UpdateDialogueNavigation(nav, 0, 0.8f, 0, 0, deadzone) != DialogueDirection::Up)
            return Fail("same direction did not re-fire after recentring");
    }

    // --- Axis-switch dominance margin -------------------------------------
    {
        DialogueNavigationState nav{};
        UpdateDialogueNavigation(nav, 0, 0.8f, 0, 0, deadzone); // engage Up
        if (UpdateDialogueNavigation(nav, 0.71f, 0.60f, 0, 0, deadzone) != DialogueDirection::Neutral)
            return Fail("axis switch fired below the dominance margin");
        if (UpdateDialogueNavigation(nav, 0.73f, 0.60f, 0, 0, deadzone) != DialogueDirection::Right)
            return Fail("axis switch did not fire above the dominance margin");
    }

    // --- Sub-deadzone input fires nothing ---------------------------------
    {
        DialogueNavigationState nav{};
        if (UpdateDialogueNavigation(nav, 0.3f, 0.3f, 0, 0, deadzone) != DialogueDirection::Neutral)
            return Fail("sub-deadzone stateful input fired a direction");
    }

    std::puts("dialogue navigation tests passed");
    return 0;
}
