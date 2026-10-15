#pragma once

#include "riddick_vr_protocol.h"
#include <cstdint>

namespace RiddickVR {

struct CombatStanceState {
    bool active = false;
    bool closingBoth = false;
    uint32_t closeStartTime = 0;
};

struct CombatStanceIntent {
    bool entered = false;
    bool exited = false;
    bool active = false;
};

inline CombatStanceIntent UpdateCombatStance(CombatStanceState& state,
        const RiddickVRControllerInput& left, const RiddickVRControllerInput& right,
        uint32_t now) {
    constexpr float ENTER_THRESHOLD = 0.85f;
    constexpr float EXIT_THRESHOLD = 0.60f;
    constexpr uint32_t ENTER_HOLD_MS = 100;
    CombatStanceIntent out;
    const bool tracking = left.isConnected && right.isConnected &&
                          left.gripPose.isValid && right.gripPose.isValid;
    if (state.active) {
        if (!tracking || left.trigger < EXIT_THRESHOLD || right.trigger < EXIT_THRESHOLD) {
            state.active = false;
            state.closingBoth = false;
            state.closeStartTime = 0;
            out.exited = true;
        }
    } else if (tracking && left.trigger >= ENTER_THRESHOLD && right.trigger >= ENTER_THRESHOLD) {
        if (!state.closingBoth) {
            state.closingBoth = true;
            state.closeStartTime = now;
        } else if (now - state.closeStartTime >= ENTER_HOLD_MS) {
            state.active = true;
            state.closingBoth = false;
            state.closeStartTime = 0;
            out.entered = true;
        }
    } else {
        state.closingBoth = false;
        state.closeStartTime = 0;
    }
    out.active = state.active;
    return out;
}

} // namespace RiddickVR
