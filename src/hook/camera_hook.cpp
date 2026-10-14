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
    static float s_lastHmdYaw = 0.0f;
    static float s_lastHmdPitch = 0.0f;
    static float s_recoilKickPitch = 0.0f;

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
            s_lastHmdYaw = currentYaw;

            // Pitch (rotation around X axis: up/down)
            float sinPitch = 2.0f * (qw * qx - qy * qz);
            float currentPitch = 0.0f;
            if (fabsf(sinPitch) >= 1.0f) {
                currentPitch = copysignf(1.5707963f, sinPitch);
            } else {
                currentPitch = asinf(sinPitch);
            }
            s_lastHmdPitch = currentPitch;

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

                            // Apply transient muzzle climb recoil kick (exponential recovery over ~120ms)
                            ctrlPitch += s_recoilKickPitch;
                            s_recoilKickPitch *= 0.82f;

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
    static bool s_isDeepProne = false;
    static bool s_eyeshineActive = false;
    static bool s_flashlightActive = false;

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

        // Deep Prone / Laser Tripwire Limbo Evasion (ADR 0030)
        bool wantProne = false;
        if (s_standingEyeHeight > 0.2f) {
            wantProne = (heightDrop > 0.48f) || (heightRatio < 0.65f);
        } else {
            wantProne = (currentY < -0.45f);
        }

        if (wantProne != s_isDeepProne) {
            s_isDeepProne = wantProne;
            if (s_isDeepProne) {
                VR_LOG_INFO("Stealth Stance: DEEP PRONE / LASER TRIPWIRE LIMBO engaged (Current Y: %.2fm, Drop: %.2fm)", currentY, heightDrop);
                vrState.leftHapticPulse = 0.25f;
            } else {
                VR_LOG_INFO("Stealth Stance: DEEP PRONE disengaged");
            }
        }
    }

    // 1b. Adaptive Stealth Wall-Peeking & Dynamic Torso Leaning (ADR 0004 & 0010)
    // Tracks horizontal head displacement relative to calibrated torso base origin:
    // - Deadband [0, 0.38m]: Riddick's floor collision capsule remains safely behind wall cover while head peeks around corners
    // - Lateral lean detection (|Lean_right| > 0.15m): Signals stealth corner peek stance with subtle tactile haptic cue (0.15)
    // - Displacement > 0.38m: Torso origin smoothly tracks player translation to prevent roomscale detachment
    static float s_torsoBaseX = 0.0f;
    static float s_torsoBaseZ = 0.0f;
    static bool s_hasTorsoBase = false;
    static bool s_isStealthPeeking = false;

    if (vrState.hmdPose.isValid) {
        float hmdX = vrState.hmdPose.position.x;
        float hmdZ = vrState.hmdPose.position.z;

        if (!s_hasTorsoBase || vrState.shouldRecenter) {
            s_torsoBaseX = hmdX;
            s_torsoBaseZ = hmdZ;
            s_hasTorsoBase = true;
            s_isStealthPeeking = false;
        } else {
            // Horizontal displacement from torso base
            float dispX = hmdX - s_torsoBaseX;
            float dispZ = hmdZ - s_torsoBaseZ;
            float horizDist = sqrtf(dispX * dispX + dispZ * dispZ);

            // Decompose into lateral (right/left) and longitudinal (forward/back) lean using stabilized heading
            float cosY = cosf(s_lastHmdYaw);
            float sinY = sinf(s_lastHmdYaw);
            // Right vector is (cosY, -sinY), Forward vector is (-sinY, -cosY)
            float leanRight = dispX * cosY - dispZ * sinY;
            float leanForward = -dispX * sinY - dispZ * cosY;

            // Lateral corner peek detection threshold: 15cm lateral head shift
            bool wantPeek = (fabsf(leanRight) > 0.15f);
            if (wantPeek != s_isStealthPeeking) {
                s_isStealthPeeking = wantPeek;
                if (s_isStealthPeeking) {
                    VR_LOG_INFO("Stealth Stance: CORNER WALL-PEEK engaged (LeanRight: %.2fm, LeanFwd: %.2fm, Crouched: %s)",
                        leanRight, leanForward, s_isPhysicallyCrouched ? "YES" : "NO");
                    vrState.leftHapticPulse = 0.15f; // Subtle stealth confirmation pulse
                } else {
                    VR_LOG_INFO("Stealth Stance: CORNER WALL-PEEK disengaged");
                }
            }

            // Exceeding 38cm roomscale lean: softly slide torso origin along with player
            constexpr float MAX_LEAN_RADIUS = 0.38f;
            if (horizDist > MAX_LEAN_RADIUS) {
                float excess = horizDist - MAX_LEAN_RADIUS;
                float dirX = dispX / horizDist;
                float dirZ = dispZ / horizDist;
                s_torsoBaseX += dirX * excess;
                s_torsoBaseZ += dirZ * excess;
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

    // Prone crawl pacing (ADR 0030): rhythmic tactile crawl heartbeat in deep prone
    static DWORD s_lastPronePulseTime = 0;
    if (s_isDeepProne && (wantW || wantS || wantA || wantD) && (now - s_lastPronePulseTime > 600)) {
        s_lastPronePulseTime = now;
        vrState.leftHapticPulse = 0.15f; // Subtle stealth crawl pulse (85 Hz)
    }

    // EVA Spacewalk Magnetic Boots Stride Cadence (ADR 0040)
    static DWORD s_lastMagBootStepTime = 0;
    static bool s_magBootLeftFoot = false;
    if (!s_isPhysicallyCrouched && !s_isDeepProne && !isClimbingActive && (wantW || wantS || wantA || wantD) && (now - s_lastMagBootStepTime > 520)) {
        s_lastMagBootStepTime = now;
        s_magBootLeftFoot = !s_magBootLeftFoot;
        if (s_magBootLeftFoot) {
            vrState.leftHapticPulse = 0.35f; // Low-frequency magnetic boot lock (75 Hz)
        } else {
            vrState.rightHapticPulse = 0.35f;
        }
    }

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

    // 5. Physical Melee Combat Gestures (Jabs, Hooks, Uppercuts, Ulak Cross-Slashes, Shiv Executions, Counter-Parries)
    if (!isClimbingActive && (now - s_lastPunchTime > 260)) {
        bool dualCrossSlash = (rVel.x < -1.1f && lVel.x > 1.1f) || (rVel.x > 1.1f && lVel.x < -1.1f);

        // 5a. Physical Parry / Counter-Disarm Thrust (ADR 0018)
        // In Starbreeze engine: Left Click while Right Click (Block) is held executes Counter-Disarm!
        // When in active Boxer Guard (s_currentBlockEngaged), driving fists forward into an attack (Vz < -1.15 m/s)
        // executes the reactive counter-strike / disarm without canceling block mode.
        bool isForwardParry = s_currentBlockEngaged &&
            ((rVel.z < -1.15f && rightLocal.z < -0.15f) || (lVel.z < -1.15f && leftLocal.z < -0.15f));

        if (isForwardParry) {
            s_lastPunchTime = now;
            VR_LOG_INFO("Physical Gesture: PARRY / COUNTER-STRIKE THRUST (Rz: %.2f, Lz: %.2f while Blocking)", rVel.z, lVel.z);
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            vrState.rightHapticPulse = 0.95f; // High-frequency bone-cracking parry shockwave (280 Hz)
            vrState.leftHapticPulse = 0.95f;
        } else if (dualCrossSlash || rSpeed > 1.8f || lSpeed > 1.8f) {
            // Immediately disengage block if currently blocking so attacks are not canceled or stuck
            if (s_currentBlockEngaged) {
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                s_currentBlockEngaged = false;
                s_physicalBlockEngaged = false;
                s_guardPoseHoldTimer = 0;
                s_guardPoseReleaseTimer = 0;
            }

            s_lastPunchTime = now;
            const auto& activeVel = (rSpeed > lSpeed) ? rVel : lVel;
            const auto& activeLocal = (rSpeed > lSpeed) ? rightLocal : leftLocal;

            // Security Drone EMP Overload & Chassis Snap (ADR 0058)
            bool dualGrappleDrone = (rightLocal.z < -0.20f && leftLocal.z < -0.20f) &&
                                    (rightLocal.z > -0.45f && leftLocal.z > -0.45f) &&
                                    (vrState.rightController.grip > 0.65f && vrState.leftController.grip > 0.65f);
            bool dualTorqueSnap = (fabsf(lVel.y - rVel.y) > 1.20f) || (fabsf(lVel.x - rVel.x) > 1.20f);
            bool droneEmpSnap = dualGrappleDrone && dualTorqueSnap;

            // Dual-Hand Riot Shield Concussive Bash (ADR 0052)
            bool dualShieldBash = s_twoHandedStabilizationActive &&
                                  (rVel.z < -1.15f && lVel.z < -1.15f) &&
                                  (rightLocal.z < -0.18f && leftLocal.z < -0.18f);

            // Dual-Hand Riot Shield Ballistic Plant (ADR 0052)
            bool dualShieldPlant = s_twoHandedStabilizationActive &&
                                   (lVel.y < -0.75f && rVel.y < -0.75f) &&
                                   (lVel.z < -0.35f && rVel.z < -0.35f);

            // Inmate Pit Brawl Stool / Chair Blunt Weapon Heavy Smash (ADR 0054)
            bool dualStoolHeavySmash = (vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f) &&
                                       (fabsf(leftLocal.x - rightLocal.x) < 0.38f) &&
                                       (lSpeed > 1.35f && rSpeed > 1.35f);

            // Pit Fighting Arena Dual-Palm Concussive Shove (ADR 0041)
            bool dualPalmShove = (rVel.z < -1.10f && lVel.z < -1.10f) &&
                                 (rightLocal.z < -0.15f && leftLocal.z < -0.15f) &&
                                 (vrState.rightController.grip > 0.40f && vrState.leftController.grip > 0.40f);

            // Vent Duct Pipe Hang & Death-From-Above Aerial Drop Ambush (ADR 0033)
            bool isLookingDown = (s_lastHmdPitch < -0.70f); // Looking steeply downward (< -40 deg) from ledge/duct
            bool isDownwardPlunge = (activeVel.y < -1.10f);

            // Two-Handed Overhead Pipe Slam / Heavy Club Crushing Strike (ADR 0026)
            bool dualOverheadSlam = (leftLocal.y > 0.10f && rightLocal.y > 0.10f) &&
                                    (vrState.leftController.grip > 0.45f && vrState.rightController.grip > 0.45f) &&
                                    (lVel.y < -1.30f && rVel.y < -1.30f);

            if (droneEmpSnap) {
                VR_LOG_INFO("Physical Combat: SECURITY DRONE EMP OVERLOAD & CHASSIS SNAP (TorqueRelY: %.2f)", fabsf(lVel.y - rVel.y));
                SendKeyTap('E');
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                vrState.rightHapticPulse = 0.95f; // Mechanical spine crunch & EMP arc wave (280 Hz)
                vrState.leftHapticPulse = 0.95f;
            } else if (dualShieldBash) {
                VR_LOG_INFO("Physical Gesture: RIOT SHIELD CONCUSSIVE BASH (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.95f; // Shield rim blunt shockwave (140 Hz)
                vrState.leftHapticPulse = 0.95f;
            } else if (dualShieldPlant) {
                VR_LOG_INFO("Physical Gesture: RIOT SHIELD BALLISTIC DECK PLANT (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('C', true); // Crouch lock cover stance
                SendKey(VK_SPACE, true);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.75f; // Deck clamp thud (80 Hz)
                vrState.leftHapticPulse = 0.75f;
            } else if (dualStoolHeavySmash) {
                VR_LOG_INFO("Physical Gesture: INMATE STOOL / BLUNT WEAPON HEAVY SMASH (LSpd: %.2f, RSpd: %.2f)", lSpeed, rSpeed);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.95f; // Bone-shattering blunt fracture (120 Hz)
                vrState.leftHapticPulse = 0.95f;
            } else if (dualPalmShove) {
                VR_LOG_INFO("Physical Gesture: PIT FIGHTING ARENA DUAL-PALM CONCUSSIVE SHOVE (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.85f; // Elastic ring boundary / concussive shockwave (140 Hz)
                vrState.leftHapticPulse = 0.85f;
            } else if (isLookingDown && isDownwardPlunge) {
                VR_LOG_INFO("Physical Gesture: DEATH-FROM-ABOVE AERIAL TAKEDOWN / DROP AMBUSH (Pitch: %.2f rad, Vy: %.2f m/s)", s_lastHmdPitch, activeVel.y);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                vrState.rightHapticPulse = 0.95f; // Bone-shattering ground impact shockwave (280 Hz)
                vrState.leftHapticPulse = 0.95f;
            } else if (dualOverheadSlam) {
                VR_LOG_INFO("Physical Gesture: TWO-HANDED OVERHEAD CRUSHING SLAM / PIPE STRIKE (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);

                vrState.rightHapticPulse = 0.95f; // Heavy blunt concussive shockwave (120 Hz)
                vrState.leftHapticPulse = 0.95f;
            } else if (dualCrossSlash) {
                VR_LOG_INFO("Physical Gesture: DUAL ULAK SCISSOR CROSS-SLASH (Rx: %.2f, Lx: %.2f)", rVel.x, lVel.x);
                SendKey('A', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('A', false);

                SendKey('D', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('D', false);

                vrState.rightHapticPulse = 0.95f;
                vrState.leftHapticPulse = 0.95f;
            } else if (activeVel.y < -1.35f && (activeLocal.y > -0.10f || s_isPhysicallyCrouched)) {
                VR_LOG_INFO("Physical Gesture: OVERHEAD SHIV PLUNGE / FINISHER (Vy: %.2f m/s, Y: %.2fm)", activeVel.y, activeLocal.y);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);

                if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.95f;
                else vrState.leftHapticPulse = 0.95f;
            } else if ((s_isPhysicallyCrouched || s_isStealthPeeking) && activeLocal.z < -0.25f && (activeVel.z < -1.1f || fabsf(activeVel.x) > 1.1f)) {
                VR_LOG_INFO("Physical Gesture: STEALTH TAKEDOWN / THROAT-SLIT initiated (Vz: %.2f, Vx: %.2f)", activeVel.z, activeVel.x);
                SendKeyTap('E');
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

                if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.90f;
                else vrState.leftHapticPulse = 0.90f;
            } else if (activeVel.y > 1.2f) {
                VR_LOG_INFO("Physical Gesture: UPPERCUT detected (Speed: %.2f m/s)", activeVel.y);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);

                if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.9f;
                else vrState.leftHapticPulse = 0.9f;
            } else if (activeVel.x > 1.2f) {
                VR_LOG_INFO("Physical Gesture: RIGHT HOOK / BLADE SLASH detected (Speed: %.2f m/s)", activeVel.x);
                SendKey('D', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('D', false);

                if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.9f;
                else vrState.leftHapticPulse = 0.9f;
            } else if (activeVel.x < -1.2f) {
                VR_LOG_INFO("Physical Gesture: LEFT HOOK / BLADE SLASH detected (Speed: %.2f m/s)", activeVel.x);
                SendKey('A', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('A', false);

                if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.9f;
                else vrState.leftHapticPulse = 0.9f;
            } else {
                bool isBatonArcJab = (vrState.rightController.trigger > 0.60f);
                if (isBatonArcJab) {
                    VR_LOG_INFO("Physical Gesture: RIOT BATON ELECTRIC ARC SHOCK & JAB (Speed: %.2f m/s)", rSpeed);
                    SendKey('W', true);
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                    SendKey('W', false);
                    vrState.rightHapticPulse = 0.90f; // High-voltage arc discharge sizzle (260 Hz)
                } else {
                    VR_LOG_INFO("Physical Gesture: STRAIGHT PUNCH / LUNGING BLADE THRUST detected (Speed: %.2f m/s)", (rSpeed > lSpeed) ? rSpeed : lSpeed);
                    SendKey('W', true);
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                    SendKey('W', false);

                    if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.9f;
                    else vrState.leftHapticPulse = 0.9f;
                }
            }
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

    // 6a. Physical Shiv Sharpening & Blade Grinding (ADR 0035)
    static DWORD s_lastHoneStrokeTime = 0;
    bool inHoneProximity = (reloadHandDist < 0.15f) && (vrState.leftController.grip > 0.40f) && (vrState.rightController.grip > 0.40f);
    if (inHoneProximity && !s_physicalBlockEngaged && !isClimbingActive) {
        float relVelY = fabsf(lVel.y - rVel.y);
        if (relVelY > 0.55f && (now - s_lastHoneStrokeTime > 250)) {
            s_lastHoneStrokeTime = now;
            VR_LOG_INFO("Physical Gesture: SHIV SHARPENING / BLADE GRINDING STROKE (RelVelY: %.2f m/s, Dist: %.2fm)", relVelY, reloadHandDist);
            vrState.leftHapticPulse = 0.55f;  // Crisp metallic scrape (220 Hz)
            vrState.rightHapticPulse = 0.55f;
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
    bool isHarriesStance = s_flashlightActive && (twoHandDist < 0.18f) && (vrState.leftController.grip > 0.35f);
    bool wantTwoHanded = (((twoHandDist < 0.25f) && (vrState.leftController.grip > 0.45f)) || isHarriesStance) &&
                         !s_physicalBlockEngaged &&
                         !isReloading &&
                         !isClimbingActive;

    if (wantTwoHanded != s_twoHandedStabilizationActive) {
        s_twoHandedStabilizationActive = wantTwoHanded;
        if (s_twoHandedStabilizationActive) {
            VR_LOG_INFO("Two-Handed Firearm Stabilization ENGAGED (%s, HandDist: %.2fm)",
                isHarriesStance ? "Tactical Harries Flashlight Stance" : "Foregrip Hold", twoHandDist);
            vrState.leftHapticPulse = isHarriesStance ? 0.35f : 0.25f;  // Subtle haptic confirmation pulse
            vrState.rightHapticPulse = isHarriesStance ? 0.35f : 0.25f;
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
                float smoothAlpha = isHarriesStance ? 0.22f : 0.35f;

                // Sniper Rifle Diegetic Eye Relief & Bipod Bracing (ADR 0042)
                float scopeEyeDist = sqrtf(
                    powf(rightLocal.x, 2) +
                    powf(rightLocal.y, 2) +
                    powf(rightLocal.z, 2)
                );
                if (scopeEyeDist < 0.16f) {
                    smoothAlpha = 0.12f; // High-precision eye relief stabilization
                    if (lSpeed < 0.08f) {
                        smoothAlpha = 0.05f; // Bipod railing bench-rest lock
                    }
                }

                s_stabilizedAimPose.position.x += smoothAlpha * (vrState.rightController.aimPose.position.x - s_stabilizedAimPose.position.x);
                s_stabilizedAimPose.position.y += smoothAlpha * (vrState.rightController.aimPose.position.y - s_stabilizedAimPose.position.y);
                s_stabilizedAimPose.position.z += smoothAlpha * (vrState.rightController.aimPose.position.z - s_stabilizedAimPose.position.z);

                // SLERP / Normalized Lerp for aim orientation quaternion
                float dot = s_stabilizedAimPose.orientation.x * vrState.rightController.aimPose.orientation.x +
                            s_stabilizedAimPose.orientation.y * vrState.rightController.aimPose.orientation.y +
                            s_stabilizedAimPose.orientation.z * vrState.rightController.aimPose.orientation.z +
                            s_stabilizedAimPose.orientation.w * vrState.rightController.aimPose.orientation.w;

                float qSign = (dot < 0.0f) ? -1.0f : 1.0f;
                s_stabilizedAimPose.orientation.x += smoothAlpha * (qSign * vrState.rightController.aimPose.orientation.x - s_stabilizedAimPose.orientation.x);
                s_stabilizedAimPose.orientation.y += smoothAlpha * (qSign * vrState.rightController.aimPose.orientation.y - s_stabilizedAimPose.orientation.y);
                s_stabilizedAimPose.orientation.z += smoothAlpha * (qSign * vrState.rightController.aimPose.orientation.z - s_stabilizedAimPose.orientation.z);
                s_stabilizedAimPose.orientation.w += smoothAlpha * (qSign * vrState.rightController.aimPose.orientation.w - s_stabilizedAimPose.orientation.w);

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

    // 6g. Two-Handed Riot Shotgun Pump Action & Chambering (ADR 0034)
    static bool s_shotgunPumpRackedBack = false;
    static DWORD s_lastPumpActionTime = 0;
    if (s_twoHandedStabilizationActive && !isHarriesStance) {
        float relZ = leftLocal.z - rightLocal.z;
        if (!s_shotgunPumpRackedBack && relZ > 0.06f && (now - s_lastPumpActionTime > 400)) {
            s_shotgunPumpRackedBack = true;
            s_lastPumpActionTime = now;
            VR_LOG_INFO("Physical Gunplay: SHOTGUN PUMP RACKED BACK / CASING EJECTED (relZ: %.2fm)", relZ);
            vrState.leftHapticPulse = 0.55f; // Mechanical slide rack click (180 Hz)
            vrState.rightHapticPulse = 0.35f;
        } else if (s_shotgunPumpRackedBack && relZ < 0.01f) {
            s_shotgunPumpRackedBack = false;
            s_lastPumpActionTime = now;
            VR_LOG_INFO("Physical Gunplay: SHOTGUN SLIDE PUSHED FORWARD / SHELL CHAMBERED (relZ: %.2fm)", relZ);
            vrState.leftHapticPulse = 0.75f; // Heavy chamber lock snap (240 Hz)
            vrState.rightHapticPulse = 0.55f;
        }
    } else {
        s_shotgunPumpRackedBack = false;
    }

    // 6h. Mining Core Pneumatic Rock Drill & Industrial Jackhammer Rumble (ADR 0037)
    static DWORD s_lastMiningDrillTime = 0;
    if (s_twoHandedStabilizationActive && (vrState.rightController.trigger > 0.80f) && (vrState.leftController.trigger > 0.80f)) {
        if (now - s_lastMiningDrillTime > 70) {
            s_lastMiningDrillTime = now;
            vrState.rightHapticPulse = 0.95f; // Seismic subterranean percussion (65 Hz)
            vrState.leftHapticPulse = 0.95f;
        }
    }

    // 6c. Spatial Weapon Holstering & Physical Inventory Gestures (ADR 0015, 0021, 0023, 0038)
    // - Right Hand Over-Shoulder (Y > 0.15m, Z > -0.05m, grip > 0.50): Equips / Holsters Primary Weapon ('1')
    // - Right Hand Chest Bandolier (Y in [-0.22, 0.05], |X| < 0.14, Z > -0.25, grip > 0.50): Equips Grenades ('4')
    // - Right Hand Low Spine / Back Waist (Y < -0.30m, Z > -0.05m, |X| < 0.15m, grip > 0.50): Equips Heavy / SCAR Gun ('5')
    // - Right Hand Hip Draw (Y < -0.32m, X > 0.14m, grip > 0.50): Equips / Holsters Sidearm / Pistol ('2')
    // - Left Hand Hip Draw (Y < -0.32m, X < -0.14m, grip > 0.50): Equips / Draws Melee Blades / Ulaks ('3')
    // - Hand at Ankle Boot (Y < -0.65m, grip > 0.50) while Crouched: Stashes / Retrieves Contraband (ADR 0038)
    static DWORD s_lastHolsterTime = 0;
    if (!isClimbingActive && !s_physicalBlockEngaged && !s_twoHandedStabilizationActive && (now - s_lastHolsterTime > 900)) {
        bool rightOverShoulder = (rightLocal.y > 0.15f && rightLocal.z > -0.05f && vrState.rightController.grip > 0.50f);
        bool rightAtChest = (rightLocal.y >= -0.22f && rightLocal.y <= 0.05f && fabsf(rightLocal.x) < 0.14f && rightLocal.z > -0.25f && vrState.rightController.grip > 0.50f);
        bool rightLowSpine = (rightLocal.y < -0.30f && rightLocal.z > -0.05f && fabsf(rightLocal.x) < 0.15f && vrState.rightController.grip > 0.50f);
        bool rightAtHip = (rightLocal.y < -0.32f && rightLocal.x > 0.14f && vrState.rightController.grip > 0.50f);
        bool leftAtHip = (leftLocal.y < -0.32f && leftLocal.x < -0.14f && vrState.leftController.grip > 0.50f);
        bool leftAtBootSheath = (leftLocal.y < -0.60f && leftLocal.x >= -0.25f && leftLocal.x <= 0.0f && vrState.leftController.grip > 0.50f);
        bool rightAtBootSheath = (rightLocal.y < -0.60f && rightLocal.x >= -0.25f && rightLocal.x <= 0.0f && vrState.rightController.grip > 0.50f);
        bool rightAtAnkle = (rightLocal.y < -0.65f && vrState.rightController.grip > 0.50f);
        bool leftAtAnkle = (leftLocal.y < -0.65f && vrState.leftController.grip > 0.50f);

        if (rightOverShoulder) {
            s_lastHolsterTime = now;
            VR_LOG_INFO("Holster Gesture: PRIMARY WEAPON slot toggled (Over-Shoulder)");
            SendKeyTap('1');
            vrState.rightHapticPulse = 0.75f; // Solid mechanical latch pulse
        } else if (rightAtChest) {
            s_lastHolsterTime = now;
            VR_LOG_INFO("Holster Gesture: TACTICAL GRENADE slot toggled (Chest Bandolier)");
            SendKeyTap('4');
            vrState.rightHapticPulse = 0.65f; // Grenade bandolier snap pulse
        } else if (rightLowSpine) {
            s_lastHolsterTime = now;
            VR_LOG_INFO("Holster Gesture: HEAVY WEAPON / SCAR GUN slot toggled (Low Spine)");
            SendKeyTap('5');
            vrState.rightHapticPulse = 0.85f; // Heavy industrial magnetic clamp pulse
        } else if ((s_isPhysicallyCrouched || s_isDeepProne) && (leftAtBootSheath || rightAtBootSheath)) {
            s_lastHolsterTime = now;
            VR_LOG_INFO("Holster Gesture: CONCEALED SHIV DRAWN FROM BOOT SHEATH (Y: %.2fm)", leftAtBootSheath ? leftLocal.y : rightLocal.y);
            SendKeyTap('3'); // Equip shiv/knife slot
            if (leftAtBootSheath) vrState.leftHapticPulse = 0.45f; // Leather boot sheath friction click (160 Hz)
            if (rightAtBootSheath) vrState.rightHapticPulse = 0.45f;
        } else if ((s_isPhysicallyCrouched || s_isDeepProne) && (rightAtAnkle || leftAtAnkle)) {
            s_lastHolsterTime = now;
            VR_LOG_INFO("Holster Gesture: CONTRABAND ANKLE BOOT POUCH accessed (HandY: %.2fm)", rightAtAnkle ? rightLocal.y : leftLocal.y);
            SendKeyTap(VK_TAB); // Cube PDA / contraband manifest
            if (rightAtAnkle) vrState.rightHapticPulse = 0.35f; // Subtle fabric rustle (140 Hz)
            if (leftAtAnkle) vrState.leftHapticPulse = 0.35f;
        } else if (rightAtHip) {
            s_lastHolsterTime = now;
            VR_LOG_INFO("Holster Gesture: SIDEARM / PISTOL slot toggled (Right Hip)");
            SendKeyTap('2');
            vrState.rightHapticPulse = 0.65f; // Leather holster draw pulse
        } else if (leftAtHip) {
            s_lastHolsterTime = now;
            VR_LOG_INFO("Holster Gesture: MELEE / ULAK slot toggled (Left Hip)");
            SendKeyTap('3');
            vrState.leftHapticPulse = 0.70f; // Metallic blade unsheathe pulse
        }
    }

    // 6e. Tactical Grenade Priming & Pin-Pulling Gesture (ADR 0021)
    // Bringing off-hand to right hand (< 16cm) with Left Grip pulls the grenade pin (mouse LMB Down cook)
    static bool s_grenadePrimed = false;
    static DWORD s_lastPinPullTime = 0;
    if (!isClimbingActive && !s_physicalBlockEngaged && !s_twoHandedStabilizationActive) {
        float handProximity = sqrtf(
            powf(leftLocal.x - rightLocal.x, 2) +
            powf(leftLocal.y - rightLocal.y, 2) +
            powf(leftLocal.z - rightLocal.z, 2)
        );

        if (!s_grenadePrimed && handProximity < 0.16f && vrState.leftController.grip > 0.50f && (now - s_lastPinPullTime > 1200)) {
            s_lastPinPullTime = now;
            s_grenadePrimed = true;
            VR_LOG_INFO("Tactical Gesture: GRENADE PIN PULLED / PRIMED (Hand Proximity: %.2fm)", handProximity);
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0); // Begin throw cook
            vrState.leftHapticPulse = 0.75f; // Sharp metallic cotter pin friction (180 Hz)
            vrState.rightHapticPulse = 0.50f;
        } else if (s_grenadePrimed && (rSpeed > 1.2f || vrState.rightController.trigger < 0.20f)) {
            // Throw release: dynamic forward arm swing or releasing right hand
            s_grenadePrimed = false;
            VR_LOG_INFO("Tactical Gesture: GRENADE LAUNCHED / THROWN (Speed: %.2f m/s)", rSpeed);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            vrState.rightHapticPulse = 0.60f; // Throw release impulse
        }
    }

    // 6f. SCAR Rifle Air Mine Detonation (ADR 0023)
    // Squeezing off-hand trigger + grip clench while two-handed detonates armed SCAR compressed air mines!
    static bool s_scarDetonatorDown = false;
    static DWORD s_lastScarDetonateTime = 0;
    if (s_twoHandedStabilizationActive) {
        bool wantDetonate = (vrState.leftController.trigger > 0.85f && vrState.leftController.grip > 0.60f);
        if (wantDetonate && !s_scarDetonatorDown && (now - s_lastScarDetonateTime > 600)) {
            s_lastScarDetonateTime = now;
            s_scarDetonatorDown = true;
            VR_LOG_INFO("Tactical Gesture: SCAR AIR MINE DETONATION TRIGGERED (Off-Hand Clench)");
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
            vrState.leftHapticPulse = 0.95f; // Heavy explosive shockwave (280 Hz)
            vrState.rightHapticPulse = 0.95f;
        } else if (!wantDetonate) {
            s_scarDetonatorDown = false;
        }
    }

    // 6d. Interactive Environmental Manipulation, Vent Prying, and Retinal Biometrics (ADR 0016)
    // - Physical Vent Grate Prying: Both hands extended forward (Z < -0.20m) and pulling backward (Vz > +0.28 m/s) with grips held
    // - Biometric Terminal / Retinal Poke: Index pointing forward into terminal plane (Z < -0.34m, Vz < -0.28 m/s)
    static DWORD s_lastEnvInteractTime = 0;
    if (!isClimbingActive && !s_physicalBlockEngaged && !s_twoHandedStabilizationActive && (now - s_lastEnvInteractTime > 800)) {
        // Vent Grate Prying (Dual-Hand Pull)
        bool bothHandsForward = (leftLocal.z < -0.20f && rightLocal.z < -0.20f);
        bool bothGripping = (vrState.leftController.grip > 0.50f && vrState.rightController.grip > 0.50f);
        bool bothPullingBack = (lVel.z > 0.28f && rVel.z > 0.28f);

        if (bothHandsForward && bothGripping && bothPullingBack) {
            s_lastEnvInteractTime = now;
            VR_LOG_INFO("Environmental Gesture: VENTILATION GRATE PRY / DUCT OPEN executed (Lz: %.2f, Rz: %.2f)", lVel.z, rVel.z);
            SendKeyTap('E');
            SendKeyTap(VK_SPACE);
            vrState.leftHapticPulse = 0.75f; // Heavy mechanical wrenching pulse
            vrState.rightHapticPulse = 0.75f;
        } else {
            // Biometric Terminal / Retinal Scanner Poke (Single-Hand Pointed Poke)
            bool rightPointing = (vrState.rightController.trigger < 0.20f && vrState.rightController.grip > 0.40f);
            bool rightPoke = (rightLocal.z < -0.34f && rVel.z < -0.28f);

            bool leftPointing = (vrState.leftController.trigger < 0.20f && vrState.leftController.grip > 0.40f);
            bool leftPoke = (leftLocal.z < -0.34f && lVel.z < -0.28f);

            // NanoMED Medical Regeneration Station Palm Touch (Flat Palm Contact - ADR 0019)
            bool rightPalmOpen = (vrState.rightController.trigger < 0.20f && vrState.rightController.grip < 0.25f);
            bool rightPalmTouch = (rightLocal.z < -0.30f && rVel.z < -0.25f);

            bool leftPalmOpen = (vrState.leftController.trigger < 0.20f && vrState.leftController.grip < 0.25f);
            bool leftPalmTouch = (leftLocal.z < -0.30f && lVel.z < -0.25f);

            if (rightPointing && rightPoke) {
                s_lastEnvInteractTime = now;
                VR_LOG_INFO("Environmental Gesture: BIOMETRIC TERMINAL TOUCH / RETINAL SCANNER (Right Hand, Z: %.2fm)", rightLocal.z);
                SendKeyTap('E');
                vrState.rightHapticPulse = 0.40f; // Digital tactile beep
            } else if (leftPointing && leftPoke) {
                s_lastEnvInteractTime = now;
                VR_LOG_INFO("Environmental Gesture: BIOMETRIC TERMINAL TOUCH / RETINAL SCANNER (Left Hand, Z: %.2fm)", leftLocal.z);
                SendKeyTap('E');
                vrState.leftHapticPulse = 0.40f; // Digital tactile beep
            } else if (rightPalmOpen && rightPalmTouch) {
                s_lastEnvInteractTime = now;
                VR_LOG_INFO("Environmental Gesture: NANOMED MEDICAL REGENERATION STATION TOUCH (Right Palm, Z: %.2fm)", rightLocal.z);
                SendKeyTap('E');
                vrState.rightHapticPulse = 0.50f; // Bio-restorative soothing haptic wave
            } else if (leftPalmOpen && leftPalmTouch) {
                s_lastEnvInteractTime = now;
                VR_LOG_INFO("Environmental Gesture: NANOMED MEDICAL REGENERATION STATION TOUCH (Left Palm, Z: %.2fm)", leftLocal.z);
                SendKeyTap('E');
                vrState.leftHapticPulse = 0.50f; // Bio-restorative soothing haptic wave
            }

            // Cryo-Pod / Hibernation Canopy Emergency Seal Ejection (Dual Palm Push - ADR 0028)
            bool bothHandsPushing = (leftLocal.z < -0.22f && rightLocal.z < -0.22f) &&
                                    (lVel.z < -0.28f && rVel.z < -0.28f) &&
                                    (vrState.leftController.trigger < 0.30f && vrState.rightController.trigger < 0.30f);
            if (bothHandsPushing) {
                s_lastEnvInteractTime = now;
                VR_LOG_INFO("Environmental Gesture: CRYO-POD / HIBERNATION CANOPY EMERGENCY EJECTION (Lz: %.2f, Rz: %.2f)", lVel.z, rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                vrState.leftHapticPulse = 0.80f; // Pneumatic decompression surge
                vrState.rightHapticPulse = 0.80f;
            }

            // Ventilation Duct Screw Fastener Unthreading (ADR 0036)
            bool rightAtVentCorner = (rightLocal.z > -0.45f && rightLocal.z < -0.20f) &&
                                     (fabsf(rightLocal.x) > 0.12f) &&
                                     (vrState.rightController.grip > 0.45f) &&
                                     (fabsf(rVel.x) > 0.45f || fabsf(rVel.y) > 0.45f);
            if (rightAtVentCorner) {
                s_lastEnvInteractTime = now;
                VR_LOG_INFO("Environmental Gesture: VENT FASTENER UNTHREADING RATCHET (Z: %.2fm, Vx: %.2f)", rightLocal.z, rVel.x);
                SendKeyTap('E');
                vrState.rightHapticPulse = 0.45f; // Mechanical ratchet tooth click (160 Hz)
            }

            // Tactical Circuit Breaker Sabotage & Power Box Lever (ADR 0043)
            bool rightAtBreakerBox = (rightLocal.z > -0.45f && rightLocal.z < -0.20f) && (vrState.rightController.grip > 0.50f) && (rVel.y < -0.40f);
            bool leftAtBreakerBox = (leftLocal.z > -0.45f && leftLocal.z < -0.20f) && (vrState.leftController.grip > 0.50f) && (lVel.y < -0.40f);
            if ((rightAtBreakerBox || leftAtBreakerBox) && (now - s_lastEnvInteractTime > 800)) {
                s_lastEnvInteractTime = now;
                VR_LOG_INFO("Environmental Gesture: CIRCUIT BREAKER SABOTAGE / POWER CUT executed (Vy: %.2f)", rightAtBreakerBox ? rVel.y : lVel.y);
                SendKeyTap('E');
                if (rightAtBreakerBox) vrState.rightHapticPulse = 0.80f; // Heavy mechanical contactor snap (140 Hz)
                if (leftAtBreakerBox) vrState.leftHapticPulse = 0.80f;
            }

            // Manual Bulkhead Handwheel Spindle Cranking (ADR 0044)
            static DWORD s_lastWheelCrankTime = 0;
            bool rightAtWheel = (rightLocal.z > -0.45f && rightLocal.z < -0.22f) && (vrState.rightController.grip > 0.50f);
            float wheelTangentialSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y);
            if (rightAtWheel && wheelTangentialSpeed > 0.50f && (now - s_lastWheelCrankTime > 250)) {
                s_lastWheelCrankTime = now;
                VR_LOG_INFO("Environmental Gesture: BULKHEAD HANDWHEEL CRANKING (Speed: %.2f m/s)", wheelTangentialSpeed);
                SendKeyTap('E');
                vrState.rightHapticPulse = 0.45f; // Hydraulic dogging ratchet click (160 Hz)
            }

            // Atmospheric Decompression Blast Shield Overhead Pull (ADR 0046)
            bool bothHandsOverhead = (leftLocal.y > 0.25f && rightLocal.y > 0.25f);
            bool bothGrippingShield = (vrState.leftController.grip > 0.50f && vrState.rightController.grip > 0.50f);
            bool bothPullingShieldDown = (lVel.y < -0.50f && rVel.y < -0.50f);
            if (bothHandsOverhead && bothGrippingShield && bothPullingShieldDown && (now - s_lastEnvInteractTime > 800)) {
                s_lastEnvInteractTime = now;
                VR_LOG_INFO("Environmental Gesture: EMERGENCY BLAST SHIELD LOWERED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKeyTap('E');
                vrState.leftHapticPulse = 0.95f; // Heavy bulkhead slam impact (120 Hz)
                vrState.rightHapticPulse = 0.95f;
            }

            // Sub-Level Mining Cart Handbrake & Throttle (ADR 0048)
            static DWORD s_lastCartRumbleTime = 0;
            bool rightAtCartLever = (rightLocal.z > -0.35f && rightLocal.z < -0.15f) && (vrState.rightController.grip > 0.50f);
            if (rightAtCartLever) {
                if (rVel.z < -0.35f) {
                    SendKey('W', true); // Open cart throttle
                    SendKey('S', false);
                } else if (rVel.z > 0.35f) {
                    SendKey('S', true); // Friction handbrake
                    SendKey('W', false);
                }
                // Continuous rail shudder haptics
                if (now - s_lastCartRumbleTime > 180) {
                    s_lastCartRumbleTime = now;
                    vrState.rightHapticPulse = 0.40f; // Steel wheel rail shudder (60 Hz)
                }
            }

            // Heavy Mech Cockpit Canopy Slam (ADR 0049)
            bool bothOverheadMech = (leftLocal.y > 0.20f && rightLocal.y > 0.20f) &&
                                    (vrState.leftController.grip > 0.50f && vrState.rightController.grip > 0.50f) &&
                                    (lVel.y < -0.45f && rVel.y < -0.45f);
            if (bothOverheadMech && (now - s_lastEnvInteractTime > 800)) {
                s_lastEnvInteractTime = now;
                VR_LOG_INFO("Environmental Gesture: MECH COCKPIT CANOPY SLAMMED & SEALED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKeyTap('E');
                vrState.leftHapticPulse = 0.85f; // Pneumatic hermetic lock surge (120 Hz)
                vrState.rightHapticPulse = 0.85f;
            }

            // Cryo-Pod Cryo-Glass Condensation Wipe & Frost Scraping (ADR 0051)
            static DWORD s_lastGlassWipeTime = 0;
            bool rightNearGlass = (rightLocal.z > -0.40f && rightLocal.z < -0.15f) &&
                                  (rightLocal.y > -0.20f && rightLocal.y < 0.25f) &&
                                  (vrState.rightController.grip < 0.60f);
            float rightWipeSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y);
            if (rightNearGlass && rightWipeSpeed > 0.40f && (now - s_lastGlassWipeTime > 160)) {
                s_lastGlassWipeTime = now;
                VR_LOG_INFO("Environmental Gesture: CRYO-GLASS CONDENSATION WIPE / FROST SCRAPE (Speed: %.2f m/s)", rightWipeSpeed);
                SendKeyTap('E');
                vrState.rightHapticPulse = 0.35f; // Squeaking glass friction micro-haptic (240 Hz)
            }

            // Subterranean High-Pressure Steam Pipe Wheel Valve Release (ADR 0053)
            static DWORD s_lastSteamValveTurnTime = 0;
            static int s_steamValveRotations = 0;
            bool rightAtSteamValve = (rightLocal.z > -0.42f && rightLocal.z < -0.18f) &&
                                     (rightLocal.y > -0.10f && rightLocal.y < 0.30f) &&
                                     (vrState.rightController.grip > 0.55f);
            float steamValveSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y);
            if (rightAtSteamValve && steamValveSpeed > 0.45f && (now - s_lastSteamValveTurnTime > 220)) {
                s_lastSteamValveTurnTime = now;
                s_steamValveRotations++;
                VR_LOG_INFO("Environmental Gesture: STEAM PIPE WHEEL VALVE ROTATION #%d (Speed: %.2f m/s)", s_steamValveRotations, steamValveSpeed);
                vrState.rightHapticPulse = 0.50f; // Metal valve friction tick (160 Hz)
                if (s_steamValveRotations >= 3) {
                    s_steamValveRotations = 0;
                    VR_LOG_INFO("Environmental Gesture: HIGH-PRESSURE STEAM VALVE POPPED / SCALDING DIVERSION RELEASED");
                    SendKeyTap('E');
                    SendKeyTap('Q'); // Thermal diversion signal
                    vrState.rightHapticPulse = 0.70f; // Sustained pneumatic steam vibration (110 Hz)
                }
            }

            // Inmate Pit Brawl Stool / Chair Blunt Weapon Wrench (ADR 0054)
            static DWORD s_lastStoolWrenchTime = 0;
            bool bothLowAtStool = (leftLocal.y > -0.70f && leftLocal.y < -0.30f) &&
                                  (rightLocal.y > -0.70f && rightLocal.y < -0.30f) &&
                                  (leftLocal.z > -0.45f && leftLocal.z < -0.15f) &&
                                  (rightLocal.z > -0.45f && rightLocal.z < -0.15f) &&
                                  (vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
            bool bothYankingUp = (lVel.y > 1.05f && rVel.y > 1.05f);
            if (bothLowAtStool && bothYankingUp && (now - s_lastStoolWrenchTime > 1000)) {
                s_lastStoolWrenchTime = now;
                VR_LOG_INFO("Environmental Gesture: INMATE BRAWL STOOL WRENCHED FROM FLOOR (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKeyTap('E');
                SendKeyTap('1'); // Equip improvised heavy melee slot
                vrState.leftHapticPulse = 0.85f; // Shearing bolt crack (200 Hz)
                vrState.rightHapticPulse = 0.85f;
            }

            // Heavy Automated Cargo Crane Lever Console Steering (ADR 0057)
            static DWORD s_lastCraneRumbleTime = 0;
            bool rightAtCraneConsole = (rightLocal.z > -0.40f && rightLocal.z < -0.15f) &&
                                       (rightLocal.y > -0.35f && rightLocal.y < -0.05f) &&
                                       (vrState.rightController.grip > 0.50f);
            if (rightAtCraneConsole) {
                if (rVel.z < -0.30f) {
                    SendKey('W', true); // Boom forward
                    SendKey('S', false);
                } else if (rVel.z > 0.30f) {
                    SendKey('S', true); // Boom backward
                    SendKey('W', false);
                }
                if (rVel.x < -0.30f) {
                    SendKey('A', true); // Trolley left
                    SendKey('D', false);
                } else if (rVel.x > 0.30f) {
                    SendKey('D', true); // Trolley right
                    SendKey('A', false);
                }
                if (now - s_lastCraneRumbleTime > 200) {
                    s_lastCraneRumbleTime = now;
                    vrState.rightHapticPulse = 0.35f; // Heavy hydraulic motor rumble (50 Hz)
                }
            }
        }
    }

    // 7. Tactical Eyeshine & Flashlight Gestures (ADR 0012)
    // - Right Hand Temple Tap (Y > 0.08m, X > 0.12m): Toggles Eyeshine / Night Vision ('Q') with 0.85 haptic pulse
    // - Left Hand Shoulder/Temple Tap (Y > 0.06m, X < -0.12m): Toggles Tactical Shoulder Flashlight ('F') with 0.75 haptic pulse
    static DWORD s_lastEyeshineTime = 0;
    static DWORD s_lastFlashlightTime = 0;

    if (!isClimbingActive) {
        // Right Hand -> Eyeshine Night Vision
        bool rightAtTemple = (rightLocal.y > 0.08f && rightLocal.x > 0.12f && vrState.rightController.grip > 0.5f);
        if (rightAtTemple && (now - s_lastEyeshineTime > 1200)) {
            s_lastEyeshineTime = now;
            s_eyeshineActive = !s_eyeshineActive;
            VR_LOG_INFO("Tactical Gesture: EYESHINE %s (Right Temple Tap)", s_eyeshineActive ? "ACTIVATED" : "DEACTIVATED");
            SendKeyTap('Q'); // Native Riddick Eyeshine key
            vrState.rightHapticPulse = 0.85f; // High-resonance optical buzz
        }

        // Left Hand -> Tactical Shoulder Flashlight
        bool leftAtShoulder = (leftLocal.y > 0.06f && leftLocal.x < -0.12f && vrState.leftController.grip > 0.5f);
        if (leftAtShoulder && (now - s_lastFlashlightTime > 1200)) {
            s_lastFlashlightTime = now;
            s_flashlightActive = !s_flashlightActive;
            VR_LOG_INFO("Tactical Gesture: SHOULDER FLASHLIGHT %s (Left Shoulder Tap)", s_flashlightActive ? "ACTIVATED" : "DEACTIVATED");
            SendKeyTap('F'); // Native Riddick Flashlight key
            vrState.leftHapticPulse = 0.75f; // Mechanical click pulse
        }
    }

    vrState.eyeshineActive = s_eyeshineActive;
    vrState.flashlightActive = s_flashlightActive;

    // 8. Aim & Block (Unified Physical Boxer Guard + Left Trigger Aim/Block)
    // In Riddick:
    // Right Mouse Button (ACTION_SECONDARYFIRE 241) = Guard / Block when unarmed/melee, Aim/Zoom when holding firearms.
    static bool s_rightTriggerDown = false;
    static bool s_leftGripDown = false, s_rightGripDown = false;

    // Right Trigger -> Fire / Primary Attack / UI Select Click
    bool wantRightTrigger = vrState.rightController.trigger > 0.5f;
    if (wantRightTrigger != s_rightTriggerDown) {
        mouse_event(wantRightTrigger ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        s_rightTriggerDown = wantRightTrigger;
        if (wantRightTrigger) {
            // Weapon Recoil Kick & Tactical Impulse (ADR 0013 & 0014)
            s_recoilKickPitch = s_twoHandedStabilizationActive ? 0.022f : 0.040f;
            vrState.rightHapticPulse = s_twoHandedStabilizationActive ? 0.65f : 0.82f;
            if (s_twoHandedStabilizationActive) {
                vrState.leftHapticPulse = 0.45f; // Foregrip haptic transmission
            }
        }
    }

    // Sustained Automatic Fire & Mech Rotary Cannon Haptic Feedback (ADR 0024):
    static DWORD s_triggerHoldStartTime = 0;
    static DWORD s_lastCyclicFireHaptic = 0;
    if (wantRightTrigger) {
        if (s_triggerHoldStartTime == 0) {
            s_triggerHoldStartTime = now;
        } else if (now - s_triggerHoldStartTime > 150) {
            // Automatic fire / Rotary chaingun cycling every 90ms
            if (now - s_lastCyclicFireHaptic > 90) {
                s_lastCyclicFireHaptic = now;
                vrState.rightHapticPulse = 0.68f;
                if (s_twoHandedStabilizationActive) {
                    vrState.leftHapticPulse = 0.42f; // Foregrip cyclic transmission
                }
            }
        }
    } else {
        s_triggerHoldStartTime = 0;
    }

    // Security Tranq Gun Tactile Capacitor Charging (ADR 0039)
    static DWORD s_tranqChargeStartTime = 0;
    static bool s_isChargingTranq = false;
    bool isAimingStance = (vrState.leftController.trigger > 0.40f) || s_twoHandedStabilizationActive;
    if (isAimingStance && vrState.rightController.trigger > 0.35f && vrState.rightController.trigger < 0.90f) {
        if (!s_isChargingTranq) {
            s_isChargingTranq = true;
            s_tranqChargeStartTime = now;
        }
        DWORD chargeElapsed = now - s_tranqChargeStartTime;
        float chargeRatio = (chargeElapsed < 800) ? (chargeElapsed / 800.0f) : 1.0f;
        vrState.rightHapticPulse = 0.20f + 0.50f * chargeRatio; // Escalating electrostatic capacitor hum (80 -> 260 Hz)
    } else {
        if (s_isChargingTranq) {
            s_isChargingTranq = false;
            DWORD chargeTotal = now - s_tranqChargeStartTime;
            if (chargeTotal > 450) {
                VR_LOG_INFO("Tactical Weapon: TRANQ GUN PEAK CHARGE DART DISCHARGED (Charge: %ums)", chargeTotal);
                vrState.rightHapticPulse = 0.85f; // Electric discharge snap (280 Hz)
            }
        }
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

    // 9. Left & Right Grip Mapping (Interactions, Physical Throwing, Body Dragging, Climbing, Reload - ADR 0017 & 0020)
    // Left Grip -> Grab / Secondary Interact ('E') / Physical Object Throwing / Body Dragging
    bool wantLeftGrip = !isClimbingActive && !s_twoHandedStabilizationActive && (vrState.leftController.grip > 0.5f);
    if (wantLeftGrip != s_leftGripDown) {
        if (!wantLeftGrip && s_leftGripDown) {
            // Evaluated at moment of release:
            // Fast throwing motion (lSpeed > 1.2 m/s) launches carried object forward!
            if (lSpeed > 1.2f) {
                VR_LOG_INFO("Physical Gesture: OBJECT THROW / PROPULSION executed (Speed: %.2f m/s, Vel: [%.2f, %.2f, %.2f])",
                    lSpeed, lVel.x, lVel.y, lVel.z);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                vrState.leftHapticPulse = 0.60f; // Tactile release impulse
            } else {
                SendKey('E', false); // Gentle drop at feet
            }
        } else {
            SendKey('E', true);
            vrState.leftHapticPulse = 0.35f; // Object grab latch
        }
        s_leftGripDown = wantLeftGrip;
    }

    // Inmate Gambling Dice Shaking & Roll Throwing (ADR 0045)
    static DWORD s_lastDiceRattleTime = 0;
    bool cuppedLeftHand = (vrState.leftController.grip > 0.40f && vrState.leftController.grip < 0.85f);
    if (cuppedLeftHand && lSpeed > 0.70f && !s_physicalBlockEngaged && (now - s_lastDiceRattleTime > 120)) {
        s_lastDiceRattleTime = now;
        vrState.leftHapticPulse = 0.30f; // Rapid bone-on-bone dice rattle micro-tap (160 Hz)
    }

    // Kinematic Body / Heavy Object Drag Resistance (ADR 0020):
    // When crouched and hauling an object/corpse backwards or sideways, emit periodic drag friction pulses
    static DWORD s_lastDragPulseTime = 0;
    if (s_leftGripDown && (s_isPhysicallyCrouched || s_isStealthPeeking)) {
        float moveMagnitude = fabsf(vrState.leftController.thumbstickX) + fabsf(vrState.leftController.thumbstickY);
        if (moveMagnitude > 0.25f && (now - s_lastDragPulseTime > 450)) {
            s_lastDragPulseTime = now;
            vrState.leftHapticPulse = 0.38f; // Low-frequency drag friction rumble (85 Hz)
        }
    }

    // Diegetic Collectible Inspection: Cigarette Packs & Bounty Cards (ADR 0029)
    // Holding item in off-hand up to eye level (Y > -0.05m, Z > -0.32m) engages inspection
    static bool s_inspectingItem = false;
    bool wantInspect = s_leftGripDown && (leftLocal.y > -0.05f && leftLocal.z > -0.32f);
    if (wantInspect != s_inspectingItem) {
        s_inspectingItem = wantInspect;
        if (s_inspectingItem) {
            VR_LOG_INFO("Diegetic Interaction: COLLECTIBLE INSPECTION / CIGARETTE PACK (Eye Level: %.2fm)", leftLocal.y);
            vrState.leftHapticPulse = 0.25f; // Tactile inspection buzz (180 Hz)
        }
    }

    // Interactive Guard Interrogation: Physical Throat-Grip Shakedown (ADR 0032)
    // Off-hand held forward at collar/chest height (Z in [-0.60, -0.22]m, Y in [-0.25, +0.15]m) with Left Grip
    static bool s_isThroatGripping = false;
    static DWORD s_lastShakedownPulseTime = 0;
    bool inThroatZone = s_leftGripDown && (leftLocal.z > -0.60f && leftLocal.z < -0.22f) &&
                                        (leftLocal.y > -0.25f && leftLocal.y < 0.15f) &&
                                        (fabsf(leftLocal.x) < 0.28f);
    if (inThroatZone) {
        if (!s_isThroatGripping) {
            s_isThroatGripping = true;
            VR_LOG_INFO("Diegetic Interaction: NPC THROAT / COLLAR GRIP engaged (Pos: [%.2f, %.2f, %.2f])",
                leftLocal.x, leftLocal.y, leftLocal.z);
            vrState.leftHapticPulse = 0.45f; // Throaty struggle pulse (120 Hz)
        }
        // Physical Forward/Back Shake: |lVel.z| > 0.70 m/s
        if (fabsf(lVel.z) > 0.70f && (now - s_lastShakedownPulseTime > 350)) {
            s_lastShakedownPulseTime = now;
            VR_LOG_INFO("Diegetic Interaction: PHYSICAL SHAKEDOWN GESTURE executed (VelZ: %.2f m/s)", lVel.z);
            SendKeyTap('E'); // Advance extortion/interrogation dialogue
            vrState.leftHapticPulse = 0.75f; // Compliance snap haptic (220 Hz)
        }
    } else {
        if (s_isThroatGripping) {
            s_isThroatGripping = false;
            VR_LOG_INFO("Diegetic Interaction: NPC THROAT GRIP released");
        }
    }

    // Guard Subdue Chokehold & Silent Neck Snap (ADR 0047)
    // Sneaking in crouch/prone with left arm wrapped across chest level (Z in [-0.35, -0.15]m, Y in [-0.20, +0.10]m) with Left Grip
    static bool s_isChokeholding = false;
    static DWORD s_lastChokeStruggleTime = 0;
    bool inChokeholdZone = (s_isPhysicallyCrouched || s_isStealthPeeking || s_isDeepProne) &&
                           s_leftGripDown && (leftLocal.z > -0.35f && leftLocal.z < -0.15f) &&
                           (leftLocal.y > -0.20f && leftLocal.y < 0.10f) &&
                           (fabsf(leftLocal.x) < 0.25f);
    if (inChokeholdZone) {
        if (!s_isChokeholding) {
            s_isChokeholding = true;
            VR_LOG_INFO("Stealth Takedown: REAR CHOKEHOLD GRAPPLE engaged");
            vrState.leftHapticPulse = 0.45f; // Rhythmic struggle pulse (100 Hz)
        }
        if (now - s_lastChokeStruggleTime > 300) {
            s_lastChokeStruggleTime = now;
            vrState.leftHapticPulse = 0.40f;
        }
        // Cervical Twist Finisher: rapid lateral jerk of right hand (|rVel.x| > 0.80 m/s)
        if (fabsf(rVel.x) > 0.80f || fabsf(rVel.y) > 0.80f) {
            s_isChokeholding = false;
            VR_LOG_INFO("Stealth Takedown: SILENT CERVICAL NECK SNAP FINISHER executed (Vx: %.2f m/s)", rVel.x);
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            vrState.rightHapticPulse = 0.95f; // Bone-cracking crunch (280 Hz)
            vrState.leftHapticPulse = 0.95f;
        }
    } else {
        if (s_isChokeholding) {
            s_isChokeholding = false;
            VR_LOG_INFO("Stealth Takedown: REAR CHOKEHOLD released");
        }
    }

    // Inmate Bunk Mattress Stash Search (ADR 0050)
    static DWORD s_lastBunkSearchTime = 0;
    bool leftHoldingMattress = (s_isPhysicallyCrouched || s_isDeepProne) &&
                               s_leftGripDown && (leftLocal.y > -0.55f && leftLocal.y < -0.25f);
    bool rightSweepingUnder = (rightLocal.z < -0.30f) && (fabsf(rVel.x) > 0.35f);
    if (leftHoldingMattress && rightSweepingUnder && (now - s_lastBunkSearchTime > 900)) {
        s_lastBunkSearchTime = now;
        VR_LOG_INFO("Environmental Gesture: INMATE BUNK MATTRESS STASH SEARCH (SweepVx: %.2f m/s)", rVel.x);
        SendKeyTap('E');
        vrState.rightHapticPulse = 0.60f; // Metallic discovery chime haptic (200 Hz)
        vrState.leftHapticPulse = 0.30f;
    }

    // Right Grip -> Weapon Reload ('R') (suppressed during ladder climb)
    bool wantRightGrip = !isClimbingActive && (vrState.rightController.grip > 0.5f);
    if (wantRightGrip != s_rightGripDown) {
        SendKey('R', wantRightGrip);
        s_rightGripDown = wantRightGrip;
        if (wantRightGrip) vrState.rightHapticPulse = 0.5f;
    }

    // 10. Face Buttons Mapping

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

    // BUTTON B -> JUMP (Space) / PHYSICAL CROUCH-JUMP (Space + C) / BACK / DISMISS / ESCAPE / CANCEL
    bool btnB = (btns & RIDDICK_VR_BTN_B) != 0;
    bool prevBtnB = (s_prevButtons & RIDDICK_VR_BTN_B) != 0;
    if (btnB != prevBtnB) {
        if (btnB && s_isPhysicallyCrouched) {
            // Physical Crouch-Jump / High Ledge Clearance (ADR 0025)
            VR_LOG_INFO("Movement Stance: PHYSICAL CROUCH-JUMP / MANTLE CLEARANCE executed");
            SendKey('C', true);
            SendKey(VK_SPACE, true);
            vrState.rightHapticPulse = 0.45f;
        } else {
            SendKey(VK_SPACE, btnB);      // Jump in gameplay
        }
        SendKey(VK_ESCAPE, btnB);     // Dismiss popups / menus / smokes
        SendKey(VK_BACK, btnB);       // Back in Cube UI
        SendKey(VK_TAB, btnB);        // Dismiss Cube PDA
        if (btnB && !s_isPhysicallyCrouched) vrState.rightHapticPulse = 0.3f;
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

    // Dual Thumbstick Click -> Emergency Ejection / Drone Neural Disconnect (ADR 0031)
    bool stickLeft = (btns & RIDDICK_VR_BTN_STICK_LEFT) != 0;
    bool stickRight = (btns & RIDDICK_VR_BTN_STICK_RIGHT) != 0;
    bool prevStickLeft = (s_prevButtons & RIDDICK_VR_BTN_STICK_LEFT) != 0;
    bool prevStickRight = (s_prevButtons & RIDDICK_VR_BTN_STICK_RIGHT) != 0;

    bool dualStick = stickLeft && stickRight;
    bool prevDualStick = prevStickLeft && prevStickRight;

    if (dualStick && !prevDualStick) {
        bool dualGrip = (vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
        if (dualGrip) {
            // Emergency Mech Cockpit Purge / Canopy Blow (ADR 0049)
            VR_LOG_INFO("Cockpit Systems: EMERGENCY MECH CANOPY BOLTS BLOWN / PILOT EJECTION");
            SendKeyTap(VK_ESCAPE);
            SendKeyTap('E');
            SendKeyTap(VK_SPACE);
            vrState.leftHapticPulse = 0.95f; // Explosive canopy bolt decompression
            vrState.rightHapticPulse = 0.95f;
        } else {
            VR_LOG_INFO("Cybernetic Uplink: EMERGENCY NEURAL EJECTION / DRONE DISCONNECT triggered (Dual-Stick Click)");
            SendKeyTap(VK_ESCAPE);
            SendKeyTap('E');
            vrState.leftHapticPulse = 0.85f;  // Intense static burst (280 Hz)
            vrState.rightHapticPulse = 0.85f;
        }
    } else {
        // Left Thumbstick Click -> Crouch Toggle ('C')
        if (stickLeft && !prevStickLeft) {
            SendKeyTap('C');
        }

        // Right Thumbstick Click -> Recenter HMD & Cinema Screen & Reset Standing Eye Height
        if (stickRight && !prevStickRight) {
            vrState.shouldRecenter = true;
            s_hasPrevHmdOrientation = false;
            if (vrState.hmdPose.isValid) {
                s_standingEyeHeight = vrState.hmdPose.position.y;
                VR_LOG_INFO("Right Stick Click: HMD recentered and standing eye height recalibrated to %.2fm", s_standingEyeHeight);
            }
            vrState.rightHapticPulse = 0.6f;
        }
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
