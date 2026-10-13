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

void CameraHook::Update(const RiddickVRSharedState& vrState) {
    if (!m_initialized || !vrState.vrActive) return;

    // 1. Physical Crouch Detection
    static bool s_isPhysicallyCrouched = false;
    constexpr float CROUCH_THRESHOLD = -0.35f; // 35 cm duck below standing baseline

    if (vrState.hmdPose.position.y < CROUCH_THRESHOLD) {
        if (!s_isPhysicallyCrouched) {
            s_isPhysicallyCrouched = true;
            VR_LOG_INFO("Physical crouch engaged (Y: %.2fm)", vrState.hmdPose.position.y);
            keybd_event('C', 0x2E, 0, 0);
        }
    } else {
        if (s_isPhysicallyCrouched) {
            s_isPhysicallyCrouched = false;
            VR_LOG_INFO("Physical crouch disengaged");
            keybd_event('C', 0x2E, KEYEVENTF_KEYUP, 0);
        }
    }

    // 2. Motion Controller Locomotion (Left Thumbstick -> WASD)
    static bool s_wPressed = false, s_sPressed = false, s_aPressed = false, s_dPressed = false;
    constexpr float DEADZONE = 0.25f;

    bool wantW = vrState.leftController.thumbstickY > DEADZONE;
    bool wantS = vrState.leftController.thumbstickY < -DEADZONE;
    bool wantD = vrState.leftController.thumbstickX > DEADZONE;
    bool wantA = vrState.leftController.thumbstickX < -DEADZONE;

    if (wantW != s_wPressed) {
        keybd_event('W', 0x11, wantW ? 0 : KEYEVENTF_KEYUP, 0);
        s_wPressed = wantW;
    }
    if (wantS != s_sPressed) {
        keybd_event('S', 0x1F, wantS ? 0 : KEYEVENTF_KEYUP, 0);
        s_sPressed = wantS;
    }
    if (wantA != s_aPressed) {
        keybd_event('A', 0x1E, wantA ? 0 : KEYEVENTF_KEYUP, 0);
        s_aPressed = wantA;
    }
    if (wantD != s_dPressed) {
        keybd_event('D', 0x20, wantD ? 0 : KEYEVENTF_KEYUP, 0);
        s_dPressed = wantD;
    }

    // 3. Smooth & Snap Turning (Right Thumbstick X)
    static bool s_snapTurnArmed = true;
    float stickX = vrState.rightController.thumbstickX;

    if (fabsf(stickX) > 0.75f) {
        if (s_snapTurnArmed) {
            s_snapTurnArmed = false;
            // Snap turn 45 degrees
            int snapDelta = (stickX > 0.0f) ? 350 : -350;
            mouse_event(MOUSEEVENTF_MOVE, snapDelta, 0, 0, 0);
        }
    } else if (fabsf(stickX) > DEADZONE) {
        // Smooth turn rate
        int deltaX = static_cast<int>(stickX * 16.0f);
        mouse_event(MOUSEEVENTF_MOVE, deltaX, 0, 0, 0);
    } else {
        s_snapTurnArmed = true;
    }

    // 4. Physical Melee Punching Gestures (Tier 2 ADR-0005)
    static DWORD s_lastPunchTime = 0;
    DWORD now = GetTickCount();

    const auto& rVel = vrState.rightController.gripPose.linearVelocity;
    const auto& lVel = vrState.leftController.gripPose.linearVelocity;
    float rSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y + rVel.z * rVel.z);
    float lSpeed = sqrtf(lVel.x * lVel.x + lVel.y * lVel.y + lVel.z * lVel.z);

    if (now - s_lastPunchTime > 350) { // Cooldown between physical punches
        if (rSpeed > 1.8f || lSpeed > 1.8f) {
            const auto& activeVel = (rSpeed > lSpeed) ? rVel : lVel;
            s_lastPunchTime = now;

            if (activeVel.y > 1.2f) {
                // Uppercut gesture (upward thrust) -> 'S' + Attack
                VR_LOG_INFO("Physical Gesture: UPPERCUT detected (speed: %.2f m/s)", (rSpeed > lSpeed) ? rSpeed : lSpeed);
                keybd_event('S', 0x1F, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                keybd_event('S', 0x1F, KEYEVENTF_KEYUP, 0);
            } else if (activeVel.x > 1.2f) {
                // Right hook -> 'D' + Attack
                VR_LOG_INFO("Physical Gesture: RIGHT HOOK detected");
                keybd_event('D', 0x20, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                keybd_event('D', 0x20, KEYEVENTF_KEYUP, 0);
            } else if (activeVel.x < -1.2f) {
                // Left hook -> 'A' + Attack
                VR_LOG_INFO("Physical Gesture: LEFT HOOK detected");
                keybd_event('A', 0x1E, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                keybd_event('A', 0x1E, KEYEVENTF_KEYUP, 0);
            } else {
                // Straight Jab / Cross -> 'W' + Attack
                VR_LOG_INFO("Physical Gesture: STRAIGHT PUNCH detected");
                keybd_event('W', 0x11, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                keybd_event('W', 0x11, KEYEVENTF_KEYUP, 0);
            }
        }
    }

    // 5. Guard / Block Pose Detection
    // Both hands raised near chin (y > -0.20m relative to neck) and close together (< 0.28m apart)
    static bool s_guardActive = false;
    float handDist = sqrtf(
        powf(vrState.leftController.gripPose.position.x - vrState.rightController.gripPose.position.x, 2) +
        powf(vrState.leftController.gripPose.position.y - vrState.rightController.gripPose.position.y, 2) +
        powf(vrState.leftController.gripPose.position.z - vrState.rightController.gripPose.position.z, 2)
    );

    bool wantGuard = (handDist < 0.28f) && 
                     (vrState.leftController.gripPose.position.y > -0.20f) && 
                     (vrState.rightController.gripPose.position.y > -0.20f);

    // 6. Combat Triggers & Grips
    static bool s_leftTriggerDown = false, s_rightTriggerDown = false;
    static bool s_leftGripDown = false, s_rightGripDown = false;

    // Right Trigger -> Fire / Primary Attack
    bool wantRightTrigger = vrState.rightController.trigger > 0.5f;
    if (wantRightTrigger != s_rightTriggerDown) {
        mouse_event(wantRightTrigger ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        s_rightTriggerDown = wantRightTrigger;
    }

    // Left Trigger OR Guard Pose -> Aim / Block (Right Click)
    bool wantBlock = (vrState.leftController.trigger > 0.5f) || wantGuard;
    if (wantBlock != s_leftTriggerDown) {
        mouse_event(wantBlock ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
        s_leftTriggerDown = wantBlock;
    }

    // Left Grip -> Interact ('E')
    bool wantLeftGrip = vrState.leftController.grip > 0.5f;
    if (wantLeftGrip != s_leftGripDown) {
        keybd_event('E', 0x12, wantLeftGrip ? 0 : KEYEVENTF_KEYUP, 0);
        s_leftGripDown = wantLeftGrip;
    }

    // Right Grip -> Reload ('R')
    bool wantRightGrip = vrState.rightController.grip > 0.5f;
    if (wantRightGrip != s_rightGripDown) {
        keybd_event('R', 0x13, wantRightGrip ? 0 : KEYEVENTF_KEYUP, 0);
        s_rightGripDown = wantRightGrip;
    }

    // 7. Face Buttons Mapping
    static uint32_t s_prevButtons = 0;
    uint32_t btns = vrState.rightController.buttons | vrState.leftController.buttons;

    // Button A -> Jump (Space)
    if ((btns & RIDDICK_VR_BTN_A) && !(s_prevButtons & RIDDICK_VR_BTN_A)) {
        keybd_event(VK_SPACE, 0x39, 0, 0);
    } else if (!(btns & RIDDICK_VR_BTN_A) && (s_prevButtons & RIDDICK_VR_BTN_A)) {
        keybd_event(VK_SPACE, 0x39, KEYEVENTF_KEYUP, 0);
    }

    // Button B -> Flashlight / Eyeshine ('F')
    if ((btns & RIDDICK_VR_BTN_B) && !(s_prevButtons & RIDDICK_VR_BTN_B)) {
        keybd_event('F', 0x21, 0, 0);
        keybd_event('F', 0x21, KEYEVENTF_KEYUP, 0);
    }

    // Button X -> Crouch toggle ('C')
    if ((btns & RIDDICK_VR_BTN_X) && !(s_prevButtons & RIDDICK_VR_BTN_X)) {
        keybd_event('C', 0x2E, 0, 0);
        keybd_event('C', 0x2E, KEYEVENTF_KEYUP, 0);
    }

    // Button Y -> Inventory / Holster ('1')
    if ((btns & RIDDICK_VR_BTN_Y) && !(s_prevButtons & RIDDICK_VR_BTN_Y)) {
        keybd_event('1', 0x02, 0, 0);
        keybd_event('1', 0x02, KEYEVENTF_KEYUP, 0);
    }

    s_prevButtons = btns;
}

} // namespace RiddickVR
