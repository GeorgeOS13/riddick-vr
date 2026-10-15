#include "combat_stance.h"
#include <cstdio>

static RiddickVRControllerInput Controller(float trigger) {
    RiddickVRControllerInput c{};
    c.isConnected = true;
    c.gripPose.isValid = true;
    c.trigger = trigger;
    return c;
}

static int Fail(const char* message) {
    std::fprintf(stderr, "combat stance test failed: %s\n", message);
    return 1;
}

int main() {
    using RiddickVR::CombatStanceState;
    using RiddickVR::UpdateCombatStance;
    CombatStanceState state{};
    auto left = Controller(0.90f);
    auto right = Controller(0.0f);

    if (UpdateCombatStance(state, left, right, 100).active)
        return Fail("one closed hand entered combat");

    right.trigger = 0.90f;
    auto first = UpdateCombatStance(state, left, right, 200);
    if (first.active || first.entered) return Fail("combat ignored its hold debounce");
    auto before = UpdateCombatStance(state, left, right, 299);
    if (before.active || before.entered) return Fail("combat entered before 100ms");
    auto entered = UpdateCombatStance(state, left, right, 300);
    if (!entered.active || !entered.entered) return Fail("both triggers did not enter combat at 100ms");

    left.trigger = 0.70f;
    auto held = UpdateCombatStance(state, left, right, 301);
    if (!held.active || held.exited) return Fail("hysteresis exited above the release threshold");
    left.trigger = 0.59f;
    auto exited = UpdateCombatStance(state, left, right, 302);
    if (exited.active || !exited.exited) return Fail("dropping either trigger did not exit");

    left.trigger = right.trigger = 0.90f;
    UpdateCombatStance(state, left, right, 0xFFFFFFF0u);
    auto wrapped = UpdateCombatStance(state, left, right, 0x00000054u);
    if (!wrapped.active || !wrapped.entered) return Fail("millisecond counter wrap broke debounce");

    right.isConnected = false;
    auto disconnected = UpdateCombatStance(state, left, right, 0x00000055u);
    if (disconnected.active || !disconnected.exited) return Fail("controller disconnect left combat latched");

    std::puts("combat stance tests passed");
    return 0;
}
