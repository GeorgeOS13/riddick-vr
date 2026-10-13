#include "camera_hook.h"
#include "logging.h"
#include <cmath>

namespace RiddickVR {

CameraHook& CameraHook::Instance() {
    static CameraHook s_instance;
    return s_instance;
}

bool CameraHook::Initialize() {
    if (m_initialized) return true;

    m_moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    VR_LOG_INFO("Initializing CameraHook. Game module base: 0x%p", (void*)m_moduleBase);

    if (ScanForCameraSignature()) {
        VR_LOG_INFO("Camera structure identified in game memory!");
    } else {
        VR_LOG_INFO("Dynamic camera matrix detachment & 6DOF injection active.");
    }

    m_initialized = true;
    return true;
}

bool CameraHook::ScanForCameraSignature() {
    return false; // Dynamic matrix intercept active
}

static void SendKey(WORD vk, bool isDown) {
    UINT scan = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);

    // 1. DirectInput 8 Scan Code hardware event
    INPUT inScancode = {};
    inScancode.type = INPUT_KEYBOARD;
    inScancode.ki.wVk = 0;
    inScancode.ki.wScan = static_cast<WORD>(scan);
    inScancode.ki.dwFlags = KEYEVENTF_SCANCODE | (isDown ? 0 : KEYEVENTF_KEYUP);
    SendInput(1, &inScancode, sizeof(INPUT));

    // 2. Standard Virtual Key event
    INPUT inVk = {};
    inVk.type = INPUT_KEYBOARD;
    inVk.ki.wVk = vk;
    inVk.ki.wScan = static_cast<WORD>(scan);
    inVk.ki.dwFlags = isDown ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &inVk, sizeof(INPUT));

    // 3. Win32 legacy keybd_event fallback
    keybd_event(static_cast<BYTE>(vk), static_cast<BYTE>(scan), isDown ? 0 : KEYEVENTF_KEYUP, 0);
}

static void SendKeyTap(WORD vk) {
    SendKey(vk, true);
    Sleep(15);
    SendKey(vk, false);
}

void CameraHook::StabilizeViewMatrix(float* outView, const float* inView, const RiddickVRSharedState& vrState, int eyePass) {
    if (!inView || !outView) return;

    const float* eyeMat = (eyePass == 2 /* RightEye */) ? vrState.rightEyeView.m : vrState.leftEyeView.m;

    if (!m_decoupleRoll && !m_decouplePitch) {
        // Direct multiplication without horizon stabilization
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                outView[c * 4 + r] =
                    inView[0 * 4 + r] * eyeMat[c * 4 + 0] +
                    inView[1 * 4 + r] * eyeMat[c * 4 + 1] +
                    inView[2 * 4 + r] * eyeMat[c * 4 + 2] +
                    inView[3 * 4 + r] * eyeMat[c * 4 + 3];
            }
        }
        return;
    }

    // Extract horizontal forward heading to stabilize horizon (eliminate camera roll/headbob wobble)
    float fwdX = -inView[2];
    float fwdZ = -inView[10];
    float yaw = atan2f(fwdX, fwdZ);

    float cosY = cosf(yaw);
    float sinY = sinf(yaw);

    float stabilizedBase[16] = {0};
    stabilizedBase[0] = cosY;
    stabilizedBase[1] = 0.0f;
    stabilizedBase[2] = -sinY;
    stabilizedBase[3] = 0.0f;

    stabilizedBase[4] = 0.0f;
    stabilizedBase[5] = 1.0f;
    stabilizedBase[6] = 0.0f;
    stabilizedBase[7] = 0.0f;

    stabilizedBase[8] = sinY;
    stabilizedBase[9] = 0.0f;
    stabilizedBase[10] = cosY;
    stabilizedBase[11] = 0.0f;

    stabilizedBase[12] = inView[12];
    stabilizedBase[13] = inView[13];
    stabilizedBase[14] = inView[14];
    stabilizedBase[15] = 1.0f;

    // Combine stabilized base with 6DOF eye pose
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            outView[c * 4 + r] =
                stabilizedBase[0 * 4 + r] * eyeMat[c * 4 + 0] +
                stabilizedBase[1 * 4 + r] * eyeMat[c * 4 + 1] +
                stabilizedBase[2 * 4 + r] * eyeMat[c * 4 + 2] +
                stabilizedBase[3 * 4 + r] * eyeMat[c * 4 + 3];
        }
    }
}

void CameraHook::Update(RiddickVRSharedState& vrState) {
    if (!m_initialized || !vrState.vrActive) return;

    // Detect if Windows mouse cursor is visible (Menus, Loading, Dialogue UI)
    CURSORINFO ci{ sizeof(CURSORINFO) };
    bool cursorShowing = false;
    if (GetCursorInfo(&ci)) {
        cursorShowing = (ci.flags & CURSOR_SHOWING) != 0;
    }

    uint32_t btns = vrState.rightController.buttons | vrState.leftController.buttons;
    static uint32_t s_prevButtons = 0;
    static bool s_manualCinemaToggle = false;

    // Toggle Cinema Screen Mode on Button Y (or auto-engage in menus when cursor is showing)
    if ((btns & RIDDICK_VR_BTN_Y) && !(s_prevButtons & RIDDICK_VR_BTN_Y)) {
        s_manualCinemaToggle = !s_manualCinemaToggle;
        VR_LOG_INFO("Cinema Screen Mode toggled: %s", s_manualCinemaToggle ? "ON (Virtual Screen)" : "OFF (Immersive 3D)");
    }
    vrState.cinemaMode = cursorShowing || s_manualCinemaToggle;

    // 0. Head Tracking Look Input (HMD Orientation -> In-Game View Yaw & Pitch)
    // Only apply in-game look when not in cinema menu mode
    static bool s_hasPrevHmdOrientation = false;
    static float s_prevHmdYaw = 0.0f;
    static float s_prevHmdPitch = 0.0f;
    static float s_accumulatedLookX = 0.0f;
    static float s_accumulatedLookY = 0.0f;

    if (vrState.hmdPose.isValid && !vrState.cinemaMode) {
        float qx = vrState.hmdPose.orientation.x;
        float qy = vrState.hmdPose.orientation.y;
        float qz = vrState.hmdPose.orientation.z;
        float qw = vrState.hmdPose.orientation.w;

        float norm = sqrtf(qx * qx + qy * qy + qz * qz + qw * qw);
        if (norm > 0.0001f) {
            qx /= norm;
            qy /= norm;
            qz /= norm;
            qw /= norm;

            // Yaw (rotation around Y axis: left/right)
            float currentYaw = atan2f(2.0f * (qw * qy + qz * qx), 1.0f - 2.0f * (qx * qx + qy * qy));

            // Pitch (rotation around X axis: up/down)
            float sinPitch = 2.0f * (qw * qx - qy * qz);
            float currentPitch = 0.0f;
            if (fabsf(sinPitch) >= 1.0f) {
                currentPitch = copysignf(1.5707963f, sinPitch);
            } else {
                currentPitch = asinf(sinPitch);
            }

            if (!s_hasPrevHmdOrientation || vrState.shouldRecenter) {
                s_prevHmdYaw = currentYaw;
                s_prevHmdPitch = currentPitch;
                s_accumulatedLookX = 0.0f;
                s_accumulatedLookY = 0.0f;
                s_hasPrevHmdOrientation = true;
            } else {
                float deltaYaw = currentYaw - s_prevHmdYaw;
                // Handle wrap-around across -pi / +pi boundary
                while (deltaYaw > 3.14159265f) deltaYaw -= 6.2831853f;
                while (deltaYaw < -3.14159265f) deltaYaw += 6.2831853f;

                float deltaPitch = currentPitch - s_prevHmdPitch;

                s_prevHmdYaw = currentYaw;
                s_prevHmdPitch = currentPitch;

                constexpr float SENSITIVITY_YAW = 445.6f;
                constexpr float SENSITIVITY_PITCH = 445.6f;

                float mouseDeltaX = -deltaYaw * SENSITIVITY_YAW;
                float mouseDeltaY = -deltaPitch * SENSITIVITY_PITCH;

                s_accumulatedLookX += mouseDeltaX;
                s_accumulatedLookY += mouseDeltaY;

                int moveX = static_cast<int>(s_accumulatedLookX);
                int moveY = static_cast<int>(s_accumulatedLookY);

                if (moveX != 0 || moveY != 0) {
                    s_accumulatedLookX -= moveX;
                    s_accumulatedLookY -= moveY;
                    mouse_event(MOUSEEVENTF_MOVE, moveX, moveY, 0, 0);
                }
            }
        }
    } else {
        s_hasPrevHmdOrientation = false;
    }

    // 1. Physical Crouch Detection
    static bool s_isPhysicallyCrouched = false;
    constexpr float CROUCH_THRESHOLD = -0.35f;

    if (vrState.hmdPose.position.y < CROUCH_THRESHOLD) {
        if (!s_isPhysicallyCrouched) {
            s_isPhysicallyCrouched = true;
            VR_LOG_INFO("Physical crouch engaged (Y: %.2fm)", vrState.hmdPose.position.y);
            SendKey('C', true);
        }
    } else {
        if (s_isPhysicallyCrouched) {
            s_isPhysicallyCrouched = false;
            VR_LOG_INFO("Physical crouch disengaged");
            SendKey('C', false);
        }
    }

    // 2. Motion Controller Locomotion (Left Thumbstick -> WASD)
    static bool s_wPressed = false, s_sPressed = false, s_aPressed = false, s_dPressed = false;
    constexpr float DEADZONE = 0.25f;

    bool wantW = vrState.leftController.thumbstickY > DEADZONE;
    bool wantS = vrState.leftController.thumbstickY < -DEADZONE;
    bool wantD = vrState.leftController.thumbstickX > DEADZONE;
    bool wantA = vrState.leftController.thumbstickX < -DEADZONE;

    if (wantW != s_wPressed) { SendKey('W', wantW); s_wPressed = wantW; }
    if (wantS != s_sPressed) { SendKey('S', wantS); s_sPressed = wantS; }
    if (wantA != s_aPressed) { SendKey('A', wantA); s_aPressed = wantA; }
    if (wantD != s_dPressed) { SendKey('D', wantD); s_dPressed = wantD; }

    // 3. Smooth & Snap Turning (Right Thumbstick X)
    static bool s_snapTurnArmed = true;
    float stickX = vrState.rightController.thumbstickX;

    if (fabsf(stickX) > 0.75f) {
        if (s_snapTurnArmed) {
            s_snapTurnArmed = false;
            int snapDelta = (stickX > 0.0f) ? 350 : -350;
            mouse_event(MOUSEEVENTF_MOVE, snapDelta, 0, 0, 0);
        }
    } else if (fabsf(stickX) > DEADZONE) {
        int deltaX = static_cast<int>(stickX * 16.0f);
        mouse_event(MOUSEEVENTF_MOVE, deltaX, 0, 0, 0);
    } else {
        s_snapTurnArmed = true;
    }

    // 4. Physical Melee Combat Gestures (Jabs, Hooks, Uppercuts)
    static DWORD s_lastPunchTime = 0;
    DWORD now = GetTickCount();

    const auto& rVel = vrState.rightController.gripPose.linearVelocity;
    const auto& lVel = vrState.leftController.gripPose.linearVelocity;
    float rSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y + rVel.z * rVel.z);
    float lSpeed = sqrtf(lVel.x * lVel.x + lVel.y * lVel.y + lVel.z * lVel.z);

    if (now - s_lastPunchTime > 350) {
        if (rSpeed > 1.8f || lSpeed > 1.8f) {
            const auto& activeVel = (rSpeed > lSpeed) ? rVel : lVel;
            s_lastPunchTime = now;

            if (activeVel.y > 1.2f) {
                VR_LOG_INFO("Physical Gesture: UPPERCUT detected");
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
            } else if (activeVel.x > 1.2f) {
                VR_LOG_INFO("Physical Gesture: RIGHT HOOK detected");
                SendKey('D', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('D', false);
            } else if (activeVel.x < -1.2f) {
                VR_LOG_INFO("Physical Gesture: LEFT HOOK detected");
                SendKey('A', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('A', false);
            } else {
                VR_LOG_INFO("Physical Gesture: STRAIGHT PUNCH detected");
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
            }

            if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.9f;
            else vrState.leftHapticPulse = 0.9f;
        }
    }

    // 5. Tactical Weapon Reload Gesture (F.E.A.R. VR Style)
    // Bringing left hand to right weapon (< 18cm) or reaching down to hip pouch triggers Reload ('R')
    static DWORD s_lastReloadGestureTime = 0;
    float reloadHandDist = sqrtf(
        powf(vrState.leftController.gripPose.position.x - vrState.rightController.gripPose.position.x, 2) +
        powf(vrState.leftController.gripPose.position.y - vrState.rightController.gripPose.position.y, 2) +
        powf(vrState.leftController.gripPose.position.z - vrState.rightController.gripPose.position.z, 2)
    );

    if (now - s_lastReloadGestureTime > 1200) {
        if ((reloadHandDist < 0.18f && vrState.leftController.grip > 0.3f) || 
            (vrState.leftController.gripPose.position.y < -0.38f && vrState.leftController.grip > 0.4f)) {
            s_lastReloadGestureTime = now;
            VR_LOG_INFO("Tactical Gesture: MANUAL WEAPON RELOAD triggered");
            SendKeyTap('R');
            vrState.leftHapticPulse = 0.5f;
            vrState.rightHapticPulse = 0.7f;
        }
    }

    // 6. Eyeshine / Night Vision Temple Gesture (tapping side of head)
    static DWORD s_lastEyeshineGestureTime = 0;
    bool handNearHead = (vrState.rightController.gripPose.position.y > 0.08f && fabsf(vrState.rightController.gripPose.position.x) > 0.12f && vrState.rightController.grip > 0.5f) ||
                        (vrState.leftController.gripPose.position.y > 0.08f && fabsf(vrState.leftController.gripPose.position.x) > 0.12f && vrState.leftController.grip > 0.5f);
    if (handNearHead && now - s_lastEyeshineGestureTime > 1500) {
        s_lastEyeshineGestureTime = now;
        VR_LOG_INFO("Tactical Gesture: EYESHINE toggled");
        SendKeyTap('Q'); // Native Riddick Eyeshine key
        SendKeyTap('F'); // Flashlight fallback
        vrState.rightHapticPulse = 0.8f;
        vrState.leftHapticPulse = 0.8f;
    }

    // 7. Guard / Block Pose Detection (both hands raised in front of chin)
    float guardHandDist = sqrtf(
        powf(vrState.leftController.gripPose.position.x - vrState.rightController.gripPose.position.x, 2) +
        powf(vrState.leftController.gripPose.position.y - vrState.rightController.gripPose.position.y, 2) +
        powf(vrState.leftController.gripPose.position.z - vrState.rightController.gripPose.position.z, 2)
    );

    bool wantGuard = (guardHandDist < 0.28f) && 
                     (vrState.leftController.gripPose.position.y > -0.20f) && 
                     (vrState.rightController.gripPose.position.y > -0.20f);

    // 8. Triggers & Grips
    static bool s_leftTriggerDown = false, s_rightTriggerDown = false;
    static bool s_leftGripDown = false, s_rightGripDown = false;

    // Right Trigger -> Fire / Primary Attack / UI Select Click
    bool wantRightTrigger = vrState.rightController.trigger > 0.5f;
    if (wantRightTrigger != s_rightTriggerDown) {
        mouse_event(wantRightTrigger ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        s_rightTriggerDown = wantRightTrigger;
        if (wantRightTrigger) vrState.rightHapticPulse = 0.4f;
    }

    // Left Trigger OR Guard Pose -> Aim / Block (Right Click)
    bool wantBlock = (vrState.leftController.trigger > 0.5f) || wantGuard;
    if (wantBlock != s_leftTriggerDown) {
        mouse_event(wantBlock ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
        s_leftTriggerDown = wantBlock;
    }

    // Left Grip -> Grab / Secondary Interact ('E')
    bool wantLeftGrip = vrState.leftController.grip > 0.5f;
    if (wantLeftGrip != s_leftGripDown) {
        SendKey('E', wantLeftGrip);
        s_leftGripDown = wantLeftGrip;
        if (wantLeftGrip) vrState.leftHapticPulse = 0.3f;
    }

    // Right Grip -> Weapon Reload ('R')
    bool wantRightGrip = vrState.rightController.grip > 0.5f;
    if (wantRightGrip != s_rightGripDown) {
        SendKey('R', wantRightGrip);
        s_rightGripDown = wantRightGrip;
        if (wantRightGrip) vrState.rightHapticPulse = 0.5f;
    }

    // 9. Face Buttons Mapping

    // BUTTON A -> PRIMARY INTERACT / USE / TALK / OPEN / CLIMB / MENU CONFIRM / INTRO SKIP
    bool btnA = (btns & RIDDICK_VR_BTN_A) != 0;
    bool prevBtnA = (s_prevButtons & RIDDICK_VR_BTN_A) != 0;
    if (btnA != prevBtnA) {
        SendKey('E', btnA);
        SendKey(VK_RETURN, btnA);
        SendKey(VK_SPACE, btnA);
        if (btnA) mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        else mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        if (btnA) vrState.rightHapticPulse = 0.4f;
    }

    // BUTTON B -> JUMP (Space)
    bool btnB = (btns & RIDDICK_VR_BTN_B) != 0;
    bool prevBtnB = (s_prevButtons & RIDDICK_VR_BTN_B) != 0;
    if (btnB != prevBtnB) {
        SendKey(VK_SPACE, btnB);
    }

    // BUTTON X -> OFF-HAND INTERACT ('E') & CROUCH TOGGLE ('C')
    bool btnX = (btns & RIDDICK_VR_BTN_X) != 0;
    bool prevBtnX = (s_prevButtons & RIDDICK_VR_BTN_X) != 0;
    if (btnX && !prevBtnX) {
        SendKeyTap('E');
        SendKeyTap('C');
        vrState.leftHapticPulse = 0.3f;
    }

    // Left Thumbstick Click -> Crouch Toggle ('C')
    bool stickLeft = (btns & RIDDICK_VR_BTN_STICK_LEFT) != 0;
    bool prevStickLeft = (s_prevButtons & RIDDICK_VR_BTN_STICK_LEFT) != 0;
    if (stickLeft && !prevStickLeft) {
        SendKeyTap('C');
    }

    // Right Thumbstick Click -> Recenter HMD & Cinema Screen
    bool stickRight = (btns & RIDDICK_VR_BTN_STICK_RIGHT) != 0;
    bool prevStickRight = (s_prevButtons & RIDDICK_VR_BTN_STICK_RIGHT) != 0;
    if (stickRight && !prevStickRight) {
        vrState.shouldRecenter = true;
        s_hasPrevHmdOrientation = false;
        vrState.rightHapticPulse = 0.6f;
    }

    s_prevButtons = btns;
}

} // namespace RiddickVR
