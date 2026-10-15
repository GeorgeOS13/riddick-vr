#pragma once

#include <windows.h>

namespace RiddickVR {

// Game intent names are deliberately independent of the legacy keyboard
// events used by Dark Athena. Keep the adapter here so controller input and
// future bindings do not embed VK_* values throughout gesture detection.
enum class VRAction {
    Interact,
    Confirm,
    Cancel,
    MoveForward,
    MoveBackward,
    MoveLeft,
    MoveRight,
    DialogueUp,
    DialogueDown,
    DialogueLeft,
    DialogueRight
};

constexpr WORD LegacyKeyForAction(VRAction action) {
    switch (action) {
    case VRAction::Interact:      return 'E';
    case VRAction::Confirm:       return VK_RETURN;
    case VRAction::Cancel:        return VK_ESCAPE;
    case VRAction::MoveForward:   return 'W';
    case VRAction::MoveBackward:  return 'S';
    case VRAction::MoveLeft:      return 'A';
    case VRAction::MoveRight:     return 'D';
    case VRAction::DialogueUp:    return VK_UP;
    case VRAction::DialogueDown:  return VK_DOWN;
    case VRAction::DialogueLeft:  return VK_LEFT;
    case VRAction::DialogueRight: return VK_RIGHT;
    }
    return 0;
}

static_assert(LegacyKeyForAction(VRAction::Interact) == 'E');
static_assert(LegacyKeyForAction(VRAction::DialogueUp) == VK_UP);
static_assert(LegacyKeyForAction(VRAction::DialogueDown) == VK_DOWN);
static_assert(LegacyKeyForAction(VRAction::DialogueLeft) == VK_LEFT);
static_assert(LegacyKeyForAction(VRAction::DialogueRight) == VK_RIGHT);

} // namespace RiddickVR
