#include "camera_hook.h"
#include "logging.h"
#include "opengl_proxy.h"
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

static HWND s_gameHwnd = nullptr;

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

    // 4. Direct window message posting to game window
    if (s_gameHwnd && IsWindow(s_gameHwnd)) {
        LPARAM lParam = (scan << 16) | 1;
        if (!isDown) lParam |= (1 << 30) | (1 << 31);
        PostMessageA(s_gameHwnd, isDown ? WM_KEYDOWN : WM_KEYUP, vk, lParam);
        if (isDown && (vk == VK_RETURN || vk == 'E' || vk == 'e' || vk == VK_ESCAPE || vk == VK_BACK || vk == VK_TAB || vk == VK_SPACE)) {
            PostMessageA(s_gameHwnd, WM_CHAR, vk, lParam);
        }
    }
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

    // Static orientation state for horizon stabilization
    static uint32_t s_lastFrameIndex = 0xFFFFFFFF;
    static float s_currentYaw = 0.0f;
    static float s_lastValidYaw = 0.0f;
    static bool s_hasInitializedYaw = false;

    // Only update yaw once per stereo frame (share exact same stabilized base between left and right eyes)
    if (vrState.frameIndex != s_lastFrameIndex || !s_hasInitializedYaw || vrState.shouldRecenter) {
        s_lastFrameIndex = vrState.frameIndex;

        // Extract horizontal forward heading from inView (-Z camera column in OpenGL)
        float fwdX = -inView[2];
        float fwdZ = -inView[10];
        float lenSq = fwdX * fwdX + fwdZ * fwdZ;

        float targetYaw = s_lastValidYaw;
        if (lenSq >= 0.001f) {
            // Well-defined horizontal heading
            targetYaw = atan2f(fwdX, fwdZ);
            s_lastValidYaw = targetYaw;
        } else {
            // Gimbal singularity guard: looking nearly straight up or down (e.g. vertical ladder climbs,
            // looking directly up a ventilation shaft). Smoothly retain last known valid yaw.
            targetYaw = s_lastValidYaw;
        }

        if (!s_hasInitializedYaw || vrState.shouldRecenter) {
            s_currentYaw = targetYaw;
            s_hasInitializedYaw = true;
        } else {
            // Calculate shortest angular difference on circle [-pi, +pi]
            float deltaYaw = targetYaw - s_currentYaw;
            while (deltaYaw > 3.14159265f) deltaYaw -= 6.2831853f;
            while (deltaYaw < -3.14159265f) deltaYaw += 6.2831853f;

            // Horizon angular rate limiting / smoothing during sudden animation transitions:
            // Prevents jarring camera snaps and disorienting rotational jerks when mounting ladders,
            // crawling into vents, or executing contextual stealth takedown animations.
            constexpr float MAX_YAW_STEP = 0.085f; // ~4.9 deg per frame (~300 deg/sec at 60 FPS)
            if (deltaYaw > MAX_YAW_STEP) {
                deltaYaw = MAX_YAW_STEP;
            } else if (deltaYaw < -MAX_YAW_STEP) {
                deltaYaw = -MAX_YAW_STEP;
            }

            s_currentYaw += deltaYaw;
            while (s_currentYaw > 3.14159265f) s_currentYaw -= 6.2831853f;
            while (s_currentYaw < -3.14159265f) s_currentYaw += 6.2831853f;
        }
    }

    float cosY = cosf(s_currentYaw);
    float sinY = sinf(s_currentYaw);

    // Construct perfectly horizontal base matrix (zero roll and pitch, purely yaw-stabilized)
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

    // Retain full 3D neck/camera translation from inView
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

// Transform a world-space point into HMD local space (where +X is right, +Y is up, -Z is forward)
static RiddickVRVector3 WorldToHmdLocal(const RiddickVRVector3& worldPos, const RiddickVRTrackedPose& hmdPose) {
    float dx = worldPos.x;
    float dy = worldPos.y;
    float dz = worldPos.z;

    if (hmdPose.isValid && (fabsf(hmdPose.position.x) > 0.001f || fabsf(hmdPose.position.y) > 0.5f || fabsf(hmdPose.position.z) > 0.001f)) {
        dx -= hmdPose.position.x;
        dy -= hmdPose.position.y;
        dz -= hmdPose.position.z;

        // Inverse quaternion rotation: q_inv = (-qx, -qy, -qz, qw)
        float qx = -hmdPose.orientation.x;
        float qy = -hmdPose.orientation.y;
        float qz = -hmdPose.orientation.z;
        float qw = hmdPose.orientation.w;

        float norm = sqrtf(qx * qx + qy * qy + qz * qz + qw * qw);
        if (norm > 0.0001f) {
            qx /= norm;
            qy /= norm;
            qz /= norm;
            qw /= norm;

            float tx = 2.0f * (qy * dz - qz * dy);
            float ty = 2.0f * (qz * dx - qx * dz);
            float tz = 2.0f * (qx * dy - qy * dx);

            float rx = dx + qw * tx + (qy * tz - qz * ty);
            float ry = dy + qw * ty + (qz * tx - qx * tz);
            float rz = dz + qw * tz + (qx * ty - qy * tx);

            return { rx, ry, rz };
        }
    }

    return { dx, dy, dz };
}

// Adaptive physical crouching, two-handed stabilization, and decoupled aim state (ADR 0004, 0005, 0008)
static float s_standingEyeHeight = 0.0f;
static bool s_twoHandedStabilizationActive = false;
static bool s_physicalBlockEngaged = false;
static DWORD s_lastReloadGestureTime = 0;
static bool s_isAimingOrBlocking = false;
static RiddickVRTrackedPose s_stabilizedAimPose{};
static bool s_hasStabilizedPose = false;

void CameraHook::Update(RiddickVRSharedState& vrState) {
    if (!m_initialized || !vrState.vrActive) return;

    DWORD now = GetTickCount();

    // Detect if Windows mouse cursor is visible (Menus, Loading, Dialogue UI)
    CURSORINFO ci{ sizeof(CURSORINFO) };
    bool cursorShowing = false;
    if (GetCursorInfo(&ci)) {
        cursorShowing = (ci.flags & CURSOR_SHOWING) != 0;
    }

    uint32_t btns = vrState.rightController.buttons | vrState.leftController.buttons;
    static uint32_t s_prevButtons = 0;
    static bool s_manualCinemaToggle = false;

    // Toggle Cinema Screen Mode manually on Button Y (or auto-engage in cursor menus)
    if ((btns & RIDDICK_VR_BTN_Y) && !(s_prevButtons & RIDDICK_VR_BTN_Y)) {
        s_manualCinemaToggle = !s_manualCinemaToggle;
        VR_LOG_INFO("Cinema Screen Mode toggled manually: %s", s_manualCinemaToggle ? "ON (Virtual Screen)" : "OFF (Immersive 3D)");
    }
    vrState.cinemaMode = cursorShowing || s_manualCinemaToggle;

    // 0. Decoupled Motion Controller Aim Steering & Head Tracking Look Input (ADR 0008)
    // Only apply in-game look when not in cinema menu mode
    static bool s_hasPrevHmdOrientation = false;
    static float s_prevHmdYaw = 0.0f;
    static float s_prevHmdPitch = 0.0f;
    static bool s_hasPrevCtrlAim = false;
    static float s_prevCtrlYaw = 0.0f;
    static float s_prevCtrlPitch = 0.0f;
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
                s_hasPrevCtrlAim = false;
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

                // Decoupled Motion Controller Aim Steering:
                // Extract relative orientation between HMD and weapon aim controller: Q_rel = Q_hmd^-1 * Q_ctrl
                const auto& ctrlPose = (s_twoHandedStabilizationActive && s_hasStabilizedPose) ?
                    s_stabilizedAimPose : (vrState.rightController.aimPose.isValid ? vrState.rightController.aimPose : vrState.rightController.gripPose);

                float deltaCtrlYaw = 0.0f;
                float deltaCtrlPitch = 0.0f;
                float pullYaw = 0.0f;
                float pullPitch = 0.0f;
                float wAim = 0.0f;
                float wPull = 0.0f;

                bool isReloading = (now - s_lastReloadGestureTime <= 1100);
                bool isAiming = s_isAimingOrBlocking || (vrState.leftController.trigger > 0.5f);

                if (ctrlPose.isValid && !s_physicalBlockEngaged && !isReloading) {
                    float cx = ctrlPose.orientation.x;
                    float cy = ctrlPose.orientation.y;
                    float cz = ctrlPose.orientation.z;
                    float cw = ctrlPose.orientation.w;
                    float cNorm = sqrtf(cx * cx + cy * cy + cz * cz + cw * cw);
                    if (cNorm > 0.0001f) {
                        cx /= cNorm; cy /= cNorm; cz /= cNorm; cw /= cNorm;

                        // Inverse HMD quaternion: q_inv = (-qx, -qy, -qz, qw)
                        float invHmdX = -qx;
                        float invHmdY = -qy;
                        float invHmdZ = -qz;
                        float invHmdW = qw;

                        // Q_rel = Q_hmd^-1 * Q_ctrl
                        float relW = invHmdW * cw - invHmdX * cx - invHmdY * cy - invHmdZ * cz;
                        float relX = invHmdW * cx + invHmdX * cw + invHmdY * cz - invHmdZ * cy;
                        float relY = invHmdW * cy - invHmdX * cz + invHmdY * cw + invHmdZ * cx;
                        float relZ = invHmdW * cz + invHmdX * cy - invHmdY * cx + invHmdZ * cw;

                        float relNorm = sqrtf(relW * relW + relX * relX + relY * relY + relZ * relZ);
                        if (relNorm > 0.0001f) {
                            relW /= relNorm; relX /= relNorm; relY /= relNorm; relZ /= relNorm;

                            // Relative forward vector (rotating (0, 0, -1) by Q_rel):
                            // fwdX = -2(relW * relY + relX * relZ)
                            // fwdY =  2(relW * relX - relY * relZ)
                            // fwdZ = -(1 - 2(relX^2 + relY^2))
                            float fwdX = -2.0f * (relW * relY + relX * relZ);
                            float fwdY =  2.0f * (relW * relX - relY * relZ);
                            float fwdZ = -(1.0f - 2.0f * (relX * relX + relY * relY));

                            // Relative controller aim angles in HMD view space
                            float ctrlYaw = atan2f(2.0f * (relW * relY + relZ * relX), 1.0f - 2.0f * (relX * relX + relY * relY));
                            float sinCtrlPitch = fwdY;
                            float ctrlPitch = 0.0f;
                            if (fabsf(sinCtrlPitch) >= 1.0f) {
                                ctrlPitch = copysignf(1.5707963f, sinCtrlPitch);
                            } else {
                                ctrlPitch = asinf(sinCtrlPitch);
                            }

                            if (!s_hasPrevCtrlAim) {
                                s_prevCtrlYaw = ctrlYaw;
                                s_prevCtrlPitch = ctrlPitch;
                                s_hasPrevCtrlAim = true;
                            } else {
                                deltaCtrlYaw = ctrlYaw - s_prevCtrlYaw;
                                while (deltaCtrlYaw > 3.14159265f) deltaCtrlYaw -= 6.2831853f;
                                while (deltaCtrlYaw < -3.14159265f) deltaCtrlYaw += 6.2831853f;

                                deltaCtrlPitch = ctrlPitch - s_prevCtrlPitch;

                                s_prevCtrlYaw = ctrlYaw;
                                s_prevCtrlPitch = ctrlPitch;

                                // Center deadzone (~2.0 degrees) to prevent micro-drift when holding weapon steady
                                constexpr float AIM_DEADZONE = 0.035f;
                                if (fabsf(ctrlYaw) > AIM_DEADZONE) {
                                    pullYaw = (ctrlYaw > 0.0f) ? (ctrlYaw - AIM_DEADZONE) : (ctrlYaw + AIM_DEADZONE);
                                }
                                if (fabsf(ctrlPitch) > AIM_DEADZONE) {
                                    pullPitch = (ctrlPitch > 0.0f) ? (ctrlPitch - AIM_DEADZONE) : (ctrlPitch + AIM_DEADZONE);
                                }

                                // Aim weights: tightened during ADS / Two-handed stabilization, lighter during hip fire
                                if (s_twoHandedStabilizationActive || isAiming) {
                                    wAim = 0.75f;
                                    wPull = 0.08f;
                                } else {
                                    wAim = 0.35f;
                                    wPull = 0.03f;
                                }
                            }
                        }
                    }
                } else {
                    s_hasPrevCtrlAim = false;
                }

                // Blend head tracking look with motion controller aim steering
                float combinedYaw = deltaYaw + (deltaCtrlYaw * wAim) + (pullYaw * wPull);
                float combinedPitch = deltaPitch + (deltaCtrlPitch * wAim) + (pullPitch * wPull);

                constexpr float SENSITIVITY_YAW = 445.6f;
                constexpr float SENSITIVITY_PITCH = 445.6f;

                float mouseDeltaX = -combinedYaw * SENSITIVITY_YAW;
                float mouseDeltaY = -combinedPitch * SENSITIVITY_PITCH;

                // Two-Handed Firearm Stabilization (ADR 0005): dampens micro-tremors and aiming jitter
                if (s_twoHandedStabilizationActive) {
                    constexpr float STABILIZE_DAMPING = 0.65f;
                    mouseDeltaX *= STABILIZE_DAMPING;
                    mouseDeltaY *= STABILIZE_DAMPING;
                }

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
        s_hasPrevCtrlAim = false;
    }

    // 1. Adaptive Physical Crouch Detection & Dynamic Height Calibration (ADR 0004 & 0005)
    static bool s_isPhysicallyCrouched = false;

    // Reset calibration on recenter request (e.g. via IPC flag)
    if (vrState.shouldRecenter && vrState.hmdPose.isValid) {
        s_standingEyeHeight = vrState.hmdPose.position.y;
        VR_LOG_INFO("Standing eye height calibrated on recenter: %.2fm", s_standingEyeHeight);
    }

    if (vrState.hmdPose.isValid) {
        float currentY = vrState.hmdPose.position.y;

        // Initial calibration
        if (s_standingEyeHeight <= 0.001f && currentY > 0.1f) {
            s_standingEyeHeight = currentY;
            VR_LOG_INFO("Initial standing eye height calibrated: %.2fm", s_standingEyeHeight);
        }

        // Peak tracking when standing upright: adapt if player stands taller or adjusts headset
        if (!s_isPhysicallyCrouched && currentY > s_standingEyeHeight) {
            s_standingEyeHeight = currentY;
        }

        // Calculate height drop from calibrated standing height
        float heightDrop = s_standingEyeHeight - currentY;
        float heightRatio = (s_standingEyeHeight > 0.2f) ? (currentY / s_standingEyeHeight) : 1.0f;

        // Crouch activation criteria:
        // Ducks > 28cm below standing height OR falls below 78% of standing height
        bool wantCrouch = false;
        if (s_standingEyeHeight > 0.2f) {
            if (s_isPhysicallyCrouched) {
                // Hysteresis release buffer: remain crouched until rising within 18cm of standing height
                wantCrouch = (heightDrop > 0.18f);
            } else {
                wantCrouch = (heightDrop > 0.28f) || (heightRatio < 0.78f);
            }
        } else {
            // Fallback for zero-baseline tracking spaces
            constexpr float FALLBACK_CROUCH_THRESHOLD = -0.28f;
            constexpr float FALLBACK_RELEASE_THRESHOLD = -0.18f;
            wantCrouch = s_isPhysicallyCrouched ? (currentY < FALLBACK_RELEASE_THRESHOLD) : (currentY < FALLBACK_CROUCH_THRESHOLD);
        }

        if (wantCrouch != s_isPhysicallyCrouched) {
            s_isPhysicallyCrouched = wantCrouch;
            VR_LOG_INFO("Adaptive physical crouch %s (Current Y: %.2fm, Standing Y: %.2fm, Drop: %.2fm)",
                s_isPhysicallyCrouched ? "ENGAGED" : "DISENGAGED", currentY, s_standingEyeHeight, heightDrop);
            SendKey('C', s_isPhysicallyCrouched);
            if (s_isPhysicallyCrouched) {
                vrState.leftHapticPulse = 0.2f;
            }
        }
    }

    // Transform controller positions into HMD-relative space for spatial gestures
    RiddickVRVector3 leftLocal = WorldToHmdLocal(vrState.leftController.gripPose.position, vrState.hmdPose);
    RiddickVRVector3 rightLocal = WorldToHmdLocal(vrState.rightController.gripPose.position, vrState.hmdPose);

    const auto& rVel = vrState.rightController.gripPose.linearVelocity;
    const auto& lVel = vrState.leftController.gripPose.linearVelocity;
    float rSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y + rVel.z * rVel.z);
    float lSpeed = sqrtf(lVel.x * lVel.x + lVel.y * lVel.y + lVel.z * lVel.z);

    // 2. Physical Ladder Climbing & Mantling Gestures (ADR 0009)
    // - Rung Grabbing: Controller raised above chin/shoulder level (Y > 0.15m relative to HMD) and grip squeezed (> 0.5f)
    // - Mount / Initiate: Taps 'E' and triggers 0.35 amplitude haptic pulse upon grabbing a rung
    // - Pull-to-Climb: Gripping and pulling down (Vy < -0.25 m/s) dispatches 'W' (forward/upward climb)
    // - Push-to-Descend: Gripping and pushing upward (Vy > +0.25 m/s) dispatches 'S' (backward/downward climb)
    // - Two-Handed Mantle: Gripping and pulling both hands down simultaneously triggers Space (mantle confirm) with 0.6 haptics
    static bool s_leftHoldingRung = false;
    static bool s_rightHoldingRung = false;
    static bool s_ladderClimbUp = false;
    static bool s_ladderClimbDown = false;
    static DWORD s_lastMantleTime = 0;

    // Grab initiate: hand raised above chin/shoulder level (Y > 0.15m) and grip squeezed (> 0.5f)
    if (!s_leftHoldingRung) {
        if (leftLocal.y > 0.15f && vrState.leftController.grip > 0.5f) {
            s_leftHoldingRung = true;
            VR_LOG_INFO("Physical Gesture: LADDER RUNG GRABBED (Left hand, Y: %.2fm)", leftLocal.y);
            vrState.leftHapticPulse = 0.35f;
            SendKeyTap('E');
        }
    } else {
        if (vrState.leftController.grip < 0.40f) {
            s_leftHoldingRung = false;
        }
    }

    if (!s_rightHoldingRung) {
        if (rightLocal.y > 0.15f && vrState.rightController.grip > 0.5f) {
            s_rightHoldingRung = true;
            VR_LOG_INFO("Physical Gesture: LADDER RUNG GRABBED (Right hand, Y: %.2fm)", rightLocal.y);
            vrState.rightHapticPulse = 0.35f;
            SendKeyTap('E');
        }
    } else {
        if (vrState.rightController.grip < 0.40f) {
            s_rightHoldingRung = false;
        }
    }

    // Evaluate climbing pull / push velocities
    bool lPullDown = s_leftHoldingRung && (lVel.y < -0.25f);
    bool rPullDown = s_rightHoldingRung && (rVel.y < -0.25f);
    bool lPushUp = s_leftHoldingRung && (lVel.y > 0.25f);
    bool rPushUp = s_rightHoldingRung && (rVel.y > 0.25f);

    // Two-Handed Mantle / Ledge Pull: Both hands pulling downward simultaneously
    if (lPullDown && rPullDown && (now - s_lastMantleTime > 1000)) {
        s_lastMantleTime = now;
        VR_LOG_INFO("Physical Gesture: TWO-HANDED MANTLE / LEDGE PULL executed");
        SendKeyTap(VK_SPACE);
        vrState.leftHapticPulse = 0.60f;
        vrState.rightHapticPulse = 0.60f;
    }

    // Pull-to-climb motion translation (alternating hands gives hand-over-hand climbing)
    s_ladderClimbUp = (lPullDown || rPullDown);
    s_ladderClimbDown = !s_ladderClimbUp && (lPushUp || rPushUp);

    // Subtle tactile haptic pulse during active climbing strokes
    if (s_ladderClimbUp) {
        if (lPullDown) vrState.leftHapticPulse = 0.20f;
        if (rPullDown) vrState.rightHapticPulse = 0.20f;
    } else if (s_ladderClimbDown) {
        if (lPushUp) vrState.leftHapticPulse = 0.20f;
        if (rPushUp) vrState.rightHapticPulse = 0.20f;
    }

    bool isClimbingActive = s_leftHoldingRung || s_rightHoldingRung || s_ladderClimbUp || s_ladderClimbDown;

    // 3. Motion Controller Locomotion (Left Thumbstick -> WASD) + Physical Climbing Coordination
    static bool s_wPressed = false, s_sPressed = false, s_aPressed = false, s_dPressed = false;
    constexpr float DEADZONE = 0.25f;

    bool wantW = (vrState.leftController.thumbstickY > DEADZONE) || s_ladderClimbUp;
    bool wantS = (vrState.leftController.thumbstickY < -DEADZONE) || s_ladderClimbDown;
    bool wantD = vrState.leftController.thumbstickX > DEADZONE;
    bool wantA = vrState.leftController.thumbstickX < -DEADZONE;

    if (wantW != s_wPressed) { SendKey('W', wantW); s_wPressed = wantW; }
    if (wantS != s_sPressed) { SendKey('S', wantS); s_sPressed = wantS; }
    if (wantA != s_aPressed) { SendKey('A', wantA); s_aPressed = wantA; }
    if (wantD != s_dPressed) { SendKey('D', wantD); s_dPressed = wantD; }

    // 4. Smooth & Snap Turning (Right Thumbstick X)
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

    // 4b. Dialogue & Menu Navigation (Right Thumbstick Y -> Up / Down Arrow Keys)
    static bool s_stickYUpArmed = true, s_stickYDownArmed = true;
    float stickY = vrState.rightController.thumbstickY;
    if (stickY > 0.55f) {
        if (s_stickYUpArmed) {
            s_stickYUpArmed = false;
            SendKeyTap(VK_UP);
            vrState.rightHapticPulse = 0.25f;
        }
    } else {
        s_stickYUpArmed = true;
    }
    if (stickY < -0.55f) {
        if (s_stickYDownArmed) {
            s_stickYDownArmed = false;
            SendKeyTap(VK_DOWN);
            vrState.rightHapticPulse = 0.25f;
        }
    } else {
        s_stickYDownArmed = true;
    }

    static DWORD s_lastPunchTime = 0;
    static DWORD s_guardPoseHoldTimer = 0;
    static DWORD s_guardPoseReleaseTimer = 0;
    static bool s_currentBlockEngaged = false;

    // 5. Physical Melee Combat Gestures (Jabs, Hooks, Uppercuts)
    if (!isClimbingActive && (now - s_lastPunchTime > 320)) {
        if (rSpeed > 1.8f || lSpeed > 1.8f) {
            // Immediately disengage block if currently blocking so punches are not canceled or stuck
            if (s_currentBlockEngaged) {
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                s_currentBlockEngaged = false;
                s_physicalBlockEngaged = false;
                s_guardPoseHoldTimer = 0;
                s_guardPoseReleaseTimer = 0;
            }

            s_lastPunchTime = now;
            const auto& activeVel = (rSpeed > lSpeed) ? rVel : lVel;

            if (activeVel.y > 1.2f) {
                VR_LOG_INFO("Physical Gesture: UPPERCUT detected (Speed: %.2f m/s)", activeVel.y);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
            } else if (activeVel.x > 1.2f) {
                VR_LOG_INFO("Physical Gesture: RIGHT HOOK detected (Speed: %.2f m/s)", activeVel.x);
                SendKey('D', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('D', false);
            } else if (activeVel.x < -1.2f) {
                VR_LOG_INFO("Physical Gesture: LEFT HOOK detected (Speed: %.2f m/s)", activeVel.x);
                SendKey('A', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('A', false);
            } else {
                VR_LOG_INFO("Physical Gesture: STRAIGHT PUNCH detected (Speed: %.2f m/s)", (rSpeed > lSpeed) ? rSpeed : lSpeed);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
            }

            if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.9f;
            else vrState.leftHapticPulse = 0.9f;
        }
    }

    // 5b. Physical Boxer Guard Pose Detection ("holding both fists up in front of the VR")
    // Criteria:
    // - Both controllers raised in front of chin/face level relative to HMD (y in -0.28 to +0.18)
    // - Both controllers in front of face (z in -0.42 to -0.12)
    // - Both hands close together horizontally (|x_left - x_right| < 0.35m)
    // - Both controllers gripped / fists clenched (> 0.40 grip)
    // - Hands steady (not throwing a punch, rSpeed < 1.4 m/s and lSpeed < 1.4 m/s)
    // - Outside punch execution recovery window (> 250ms since last punch)
    // - Not actively holding ladder rungs or climbing
    bool chinLevel = (leftLocal.y >= -0.28f && leftLocal.y <= 0.18f) &&
                     (rightLocal.y >= -0.28f && rightLocal.y <= 0.18f);
    bool inFrontOfFace = (leftLocal.z >= -0.42f && leftLocal.z <= -0.12f) &&
                         (rightLocal.z >= -0.42f && rightLocal.z <= -0.12f);
    bool handsClose = fabsf(leftLocal.x - rightLocal.x) < 0.35f;
    bool fistsClenched = (vrState.leftController.grip > 0.40f && vrState.rightController.grip > 0.40f);
    bool handsSteady = (rSpeed < 1.4f && lSpeed < 1.4f);
    bool notPunching = (now - s_lastPunchTime > 250);

    bool guardPoseDetected = !isClimbingActive && chinLevel && inFrontOfFace && handsClose && fistsClenched && handsSteady && notPunching;

    // Temporal Hysteresis & Debouncing:
    // Require guard pose to hold for ~60ms to engage (prevents false-triggers during punches)
    if (guardPoseDetected) {
        s_guardPoseReleaseTimer = 0;
        if (s_guardPoseHoldTimer == 0) {
            s_guardPoseHoldTimer = now;
        } else if (now - s_guardPoseHoldTimer >= 60) {
            s_physicalBlockEngaged = true;
        }
    } else {
        s_guardPoseHoldTimer = 0;
        if (s_physicalBlockEngaged) {
            // If throwing a punch, release block instantly (0ms)
            if (rSpeed > 1.5f || lSpeed > 1.5f) {
                s_physicalBlockEngaged = false;
                s_guardPoseReleaseTimer = 0;
            } else {
                // When hands drop naturally, require 80ms release buffer to eliminate jitter
                if (s_guardPoseReleaseTimer == 0) {
                    s_guardPoseReleaseTimer = now;
                } else if (now - s_guardPoseReleaseTimer >= 80) {
                    s_physicalBlockEngaged = false;
                    s_guardPoseReleaseTimer = 0;
                }
            }
        }
    }

    // 6. Tactical Weapon Reload Gestures (F.E.A.R. / Riddick VR Style)
    // Supports physical gestures:
    // A) Off-hand brought to weapon receiver / magazine well (< 20cm)
    // B) Off-hand reaching down to waist ammo pouch (y < -0.38m relative to HMD)
    float reloadHandDist = sqrtf(
        powf(leftLocal.x - rightLocal.x, 2) +
        powf(leftLocal.y - rightLocal.y, 2) +
        powf(leftLocal.z - rightLocal.z, 2)
    );

    if (!s_physicalBlockEngaged && !isClimbingActive && (now - s_lastReloadGestureTime > 1100)) {
        bool offHandAtReceiver = (reloadHandDist < 0.20f && vrState.leftController.grip > 0.35f && (leftLocal.y < -0.10f || rightLocal.z < -0.25f));
        bool handAtWaistPouch = (leftLocal.y < -0.38f && vrState.leftController.grip > 0.38f) ||
                                (rightLocal.y < -0.38f && vrState.rightController.grip > 0.50f);

        if (offHandAtReceiver || handAtWaistPouch) {
            s_lastReloadGestureTime = now;
            VR_LOG_INFO("Tactical Gesture: MANUAL WEAPON RELOAD triggered (%s, HandDist: %.2fm, LeftY: %.2f)",
                offHandAtReceiver ? "Weapon Receiver / Mag-Well" : "Waist Ammo Pouch", reloadHandDist, leftLocal.y);
            SendKeyTap('R');
            vrState.leftHapticPulse = 0.55f;
            vrState.rightHapticPulse = 0.75f;
        }
    }

    // 6b. Two-Handed Firearm Stabilization (ADR 0005)
    // Detect when player brings off-hand to steady the two-handed weapon:
    // - Distance between controller grip poses < 25cm
    // - Off-hand grip button held (leftController.grip > 0.45)
    // - Not in boxer guard or weapon reload mode
    float twoHandDist = sqrtf(
        powf(vrState.leftController.gripPose.position.x - vrState.rightController.gripPose.position.x, 2) +
        powf(vrState.leftController.gripPose.position.y - vrState.rightController.gripPose.position.y, 2) +
        powf(vrState.leftController.gripPose.position.z - vrState.rightController.gripPose.position.z, 2)
    );

    bool isReloading = (now - s_lastReloadGestureTime <= 1100);
    bool wantTwoHanded = (twoHandDist < 0.25f) &&
                         (vrState.leftController.grip > 0.45f) &&
                         !s_physicalBlockEngaged &&
                         !isReloading &&
                         !isClimbingActive;

    if (wantTwoHanded != s_twoHandedStabilizationActive) {
        s_twoHandedStabilizationActive = wantTwoHanded;
        if (s_twoHandedStabilizationActive) {
            VR_LOG_INFO("Two-Handed Firearm Stabilization ENGAGED (HandDist: %.2fm)", twoHandDist);
            vrState.leftHapticPulse = 0.25f;  // Subtle haptic confirmation pulse
            vrState.rightHapticPulse = 0.25f;
        } else {
            VR_LOG_INFO("Two-Handed Firearm Stabilization DISENGAGED");
        }
    }

    // Apply exponential smoothing / spherical interpolation on weapon aim pose when two-handed
    if (vrState.rightController.aimPose.isValid) {
        if (s_twoHandedStabilizationActive) {
            if (!s_hasStabilizedPose) {
                s_stabilizedAimPose = vrState.rightController.aimPose;
                s_hasStabilizedPose = true;
            } else {
                constexpr float SMOOTH_ALPHA = 0.35f;
                s_stabilizedAimPose.position.x += SMOOTH_ALPHA * (vrState.rightController.aimPose.position.x - s_stabilizedAimPose.position.x);
                s_stabilizedAimPose.position.y += SMOOTH_ALPHA * (vrState.rightController.aimPose.position.y - s_stabilizedAimPose.position.y);
                s_stabilizedAimPose.position.z += SMOOTH_ALPHA * (vrState.rightController.aimPose.position.z - s_stabilizedAimPose.position.z);

                // SLERP / Normalized Lerp for aim orientation quaternion
                float dot = s_stabilizedAimPose.orientation.x * vrState.rightController.aimPose.orientation.x +
                            s_stabilizedAimPose.orientation.y * vrState.rightController.aimPose.orientation.y +
                            s_stabilizedAimPose.orientation.z * vrState.rightController.aimPose.orientation.z +
                            s_stabilizedAimPose.orientation.w * vrState.rightController.aimPose.orientation.w;

                float qSign = (dot < 0.0f) ? -1.0f : 1.0f;
                s_stabilizedAimPose.orientation.x += SMOOTH_ALPHA * (qSign * vrState.rightController.aimPose.orientation.x - s_stabilizedAimPose.orientation.x);
                s_stabilizedAimPose.orientation.y += SMOOTH_ALPHA * (qSign * vrState.rightController.aimPose.orientation.y - s_stabilizedAimPose.orientation.y);
                s_stabilizedAimPose.orientation.z += SMOOTH_ALPHA * (qSign * vrState.rightController.aimPose.orientation.z - s_stabilizedAimPose.orientation.z);
                s_stabilizedAimPose.orientation.w += SMOOTH_ALPHA * (qSign * vrState.rightController.aimPose.orientation.w - s_stabilizedAimPose.orientation.w);

                float qNorm = sqrtf(s_stabilizedAimPose.orientation.x * s_stabilizedAimPose.orientation.x +
                                    s_stabilizedAimPose.orientation.y * s_stabilizedAimPose.orientation.y +
                                    s_stabilizedAimPose.orientation.z * s_stabilizedAimPose.orientation.z +
                                    s_stabilizedAimPose.orientation.w * s_stabilizedAimPose.orientation.w);
                if (qNorm > 0.0001f) {
                    s_stabilizedAimPose.orientation.x /= qNorm;
                    s_stabilizedAimPose.orientation.y /= qNorm;
                    s_stabilizedAimPose.orientation.z /= qNorm;
                    s_stabilizedAimPose.orientation.w /= qNorm;
                }

                vrState.rightController.aimPose = s_stabilizedAimPose;
            }
        } else {
            s_hasStabilizedPose = false;
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

    // 7. Aim & Block (Unified Physical Boxer Guard + Left Trigger Aim/Block)
    // In Riddick:
    // Right Mouse Button (ACTION_SECONDARYFIRE 241) = Guard / Block when unarmed/melee, Aim/Zoom when holding firearms.
    static bool s_rightTriggerDown = false;
    static bool s_leftGripDown = false, s_rightGripDown = false;

    // Right Trigger -> Fire / Primary Attack / UI Select Click
    bool wantRightTrigger = vrState.rightController.trigger > 0.5f;
    if (wantRightTrigger != s_rightTriggerDown) {
        mouse_event(wantRightTrigger ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        s_rightTriggerDown = wantRightTrigger;
        if (wantRightTrigger) vrState.rightHapticPulse = 0.4f;
    }

    // Block / Aim: Engaged physically via Boxer Guard pose OR manually via Left Trigger
    bool wantBlock = s_physicalBlockEngaged || (vrState.leftController.trigger > 0.5f);
    s_isAimingOrBlocking = wantBlock;
    if (wantBlock != s_currentBlockEngaged) {
        if (wantBlock) {
            if (now - s_lastPunchTime > 200) {
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                s_currentBlockEngaged = true;
                if (s_physicalBlockEngaged) {
                    VR_LOG_INFO("Combat State: BOXER GUARD / BLOCK MODE engaged (Physical Gesture)");
                    vrState.leftHapticPulse = 0.35f;
                    vrState.rightHapticPulse = 0.35f;
                } else {
                    VR_LOG_INFO("Combat State: AIM / BLOCK engaged (Left Trigger)");
                    vrState.leftHapticPulse = 0.3f;
                }
            }
        } else {
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
            s_currentBlockEngaged = false;
            VR_LOG_INFO("Combat State: AIM / BLOCK disengaged");
        }
    }

    // Left Grip -> Grab / Secondary Interact ('E') (suppressed during two-handed stabilization)
    bool wantLeftGrip = !s_twoHandedStabilizationActive && (vrState.leftController.grip > 0.5f);
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

    // BUTTON B -> JUMP (Space) / BACK / DISMISS / ESCAPE / CANCEL
    bool btnB = (btns & RIDDICK_VR_BTN_B) != 0;
    bool prevBtnB = (s_prevButtons & RIDDICK_VR_BTN_B) != 0;
    if (btnB != prevBtnB) {
        SendKey(VK_SPACE, btnB);      // Jump in gameplay
        SendKey(VK_ESCAPE, btnB);     // Dismiss popups / menus / smokes
        SendKey(VK_BACK, btnB);       // Back in Cube UI
        SendKey(VK_TAB, btnB);        // Dismiss Cube PDA
        if (btnB) vrState.rightHapticPulse = 0.3f;
    }

    // BUTTON X -> OFF-HAND INTERACT ('E') & CROUCH TOGGLE ('C') & CONFIRM
    bool btnX = (btns & RIDDICK_VR_BTN_X) != 0;
    bool prevBtnX = (s_prevButtons & RIDDICK_VR_BTN_X) != 0;
    if (btnX && !prevBtnX) {
        SendKeyTap('E');
        SendKeyTap(VK_RETURN);
        SendKeyTap('C');
        vrState.leftHapticPulse = 0.3f;
    }

    // Left Thumbstick Click -> Crouch Toggle ('C')
    bool stickLeft = (btns & RIDDICK_VR_BTN_STICK_LEFT) != 0;
    bool prevStickLeft = (s_prevButtons & RIDDICK_VR_BTN_STICK_LEFT) != 0;
    if (stickLeft && !prevStickLeft) {
        SendKeyTap('C');
    }

    // Right Thumbstick Click -> Recenter HMD & Cinema Screen & Reset Standing Eye Height
    bool stickRight = (btns & RIDDICK_VR_BTN_STICK_RIGHT) != 0;
    bool prevStickRight = (s_prevButtons & RIDDICK_VR_BTN_STICK_RIGHT) != 0;
    if (stickRight && !prevStickRight) {
        vrState.shouldRecenter = true;
        s_hasPrevHmdOrientation = false;
        if (vrState.hmdPose.isValid) {
            s_standingEyeHeight = vrState.hmdPose.position.y;
            VR_LOG_INFO("Right Stick Click: HMD recentered and standing eye height recalibrated to %.2fm", s_standingEyeHeight);
        }
        vrState.rightHapticPulse = 0.6f;
    }

    // Left Controller Menu Button -> Pause Menu / Dismiss / Skip Cutscene (Escape & Backspace & Tab)
    bool btnMenu = (btns & RIDDICK_VR_BTN_MENU) != 0;
    bool prevBtnMenu = (s_prevButtons & RIDDICK_VR_BTN_MENU) != 0;
    if (btnMenu && !prevBtnMenu) {
        SendKeyTap(VK_ESCAPE);
        SendKeyTap(VK_BACK);
        SendKeyTap(VK_TAB);
        SendKeyTap(VK_SPACE);
        vrState.leftHapticPulse = 0.4f;
    }

    s_prevButtons = btns;
}

} // namespace RiddickVR
