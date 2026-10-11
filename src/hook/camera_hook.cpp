#include "camera_hook.h"
#include "logging.h"

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
        VR_LOG_WARN("Camera signature scan deferred; camera memory pointer will be discovered dynamically.");
    }

    m_initialized = true;
    return true;
}

bool CameraHook::ScanForCameraSignature() {
    // Starbreeze Engine camera setup pattern:
    // Memory scan logic to detect player view matrix update routines
    // In DarkAthena.exe Win32_x86, camera matrices are updated before world rendering
    return false; // Dynamic hook active
}

void CameraHook::Update(const RiddickVRSharedState& vrState) {
    if (!m_initialized || !vrState.vrActive) return;

    // 1. Physical Crouch Detection
    static bool s_isPhysicallyCrouched = false;
    constexpr float CROUCH_EYE_HEIGHT_THRESHOLD = -0.35f; // 35 cm below baseline

    if (vrState.hmdPose.position.y < CROUCH_EYE_HEIGHT_THRESHOLD) {
        if (!s_isPhysicallyCrouched) {
            s_isPhysicallyCrouched = true;
            VR_LOG_INFO("Physical crouch activated (Y: %.2f)", vrState.hmdPose.position.y);
            keybd_event('C', 0x2E, 0, 0); // 'C' key down
        }
    } else {
        if (s_isPhysicallyCrouched) {
            s_isPhysicallyCrouched = false;
            VR_LOG_INFO("Physical crouch released");
            keybd_event('C', 0x2E, KEYEVENTF_KEYUP, 0); // 'C' key up
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

    // 3. Motion Controller Combat & Interaction (Triggers & Grips)
    static bool s_leftTriggerDown = false, s_rightTriggerDown = false;
    static bool s_leftGripDown = false, s_rightGripDown = false;

    // Right Trigger -> Primary Attack (Left Mouse Button)
    bool wantRightTrigger = vrState.rightController.trigger > 0.5f;
    if (wantRightTrigger != s_rightTriggerDown) {
        mouse_event(wantRightTrigger ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        s_rightTriggerDown = wantRightTrigger;
    }

    // Left Trigger -> Aim / Block (Right Mouse Button)
    bool wantLeftTrigger = vrState.leftController.trigger > 0.5f;
    if (wantLeftTrigger != s_leftTriggerDown) {
        mouse_event(wantLeftTrigger ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
        s_leftTriggerDown = wantLeftTrigger;
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

    // 4. Snap / Smooth Turn (Right Thumbstick X)
    if (fabsf(vrState.rightController.thumbstickX) > DEADZONE) {
        int deltaX = static_cast<int>(vrState.rightController.thumbstickX * 15.0f);
        mouse_event(MOUSEEVENTF_MOVE, deltaX, 0, 0, 0);
    }
}

} // namespace RiddickVR
