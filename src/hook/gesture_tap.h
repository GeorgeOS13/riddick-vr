#pragma once

// Track C #15: central debounced gesture key tap.
// Per-action debounce (default from INI, clamped 120-250ms) plus a dedup
// table that drops repeats of the same chord (vk + actionId) inside the
// window. Fixes keystroke flooding + duplication in melee gestures.

#include <windows.h>

namespace RiddickVR {

// Low-level multi-path key event (DirectInput scancode + VK + legacy +
// window message). Moved here from camera_hook.cpp so all gesture taps
// funnel through one choke point.
void SendKey(WORD vk, bool isDown);
void SendKeyTap(WORD vk);
void SetGestureGameHwnd(HWND hwnd);

// Debounced tap: same (vk, actionId) repeated inside debounceMs is dropped.
// Pass debounceMsOverride = 0 to use the INI default (GestureDebounceMs).
void SendGestureTap(WORD vk, const char* actionId, DWORD debounceMsOverride = 0);

} // namespace RiddickVR
