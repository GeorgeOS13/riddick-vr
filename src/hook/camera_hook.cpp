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
    static float s_lastHmdDeltaYaw = 0.0f;
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
                s_lastHmdDeltaYaw = deltaYaw;

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

    // Dark Athena EVA Solar Flare Shadow Occlusion Stance (ADR 0119)
    static DWORD s_lastSolarShadowTuckTime = 0;
    static bool s_isSolarShadowTuck = false;
    bool handsTuckedIn = (fabsf(leftLocal.x) < 0.14f && fabsf(rightLocal.x) < 0.14f) &&
                         (leftLocal.z > -0.20f && rightLocal.z > -0.20f) &&
                         (vrState.leftController.grip > 0.50f && vrState.rightController.grip > 0.50f);
    bool wantSolarShadowTuck = (s_isPhysicallyCrouched || s_isDeepProne) && handsTuckedIn;
    if (wantSolarShadowTuck != s_isSolarShadowTuck) {
        s_isSolarShadowTuck = wantSolarShadowTuck;
        if (s_isSolarShadowTuck && (now - s_lastSolarShadowTuckTime > 1500)) {
            s_lastSolarShadowTuckTime = now;
            VR_LOG_INFO("Environmental Stealth: EVA SOLAR FLARE SHADOW OCCLUSION TUCK engaged (HandsX: [%.2f, %.2f])", leftLocal.x, rightLocal.x);
            SendKeyTap('C');
            SendKeyTap(VK_SPACE);
            SendKeyTap('Q');
            vrState.leftHapticPulse = 0.35f;  // Cardiovascular shadow stabilization pulse (60 Hz)
            vrState.rightHapticPulse = 0.35f;
        }
    }

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

    // Inmate Bunk Overhead Pipe Chin-Up & Ledge Pull Stance (ADR 0090)
    static DWORD s_lastPipeChinUpTime = 0;
    bool overheadHands = (leftLocal.y > 0.22f && rightLocal.y > 0.22f);
    bool bothGripsClenched = (vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
    bool chinUpPullDown = (lVel.y < -0.50f && rVel.y < -0.50f);
    if (overheadHands && bothGripsClenched && chinUpPullDown && (now - s_lastPipeChinUpTime > 1800)) {
        s_lastPipeChinUpTime = now;
        VR_LOG_INFO("Physical Gesture: INMATE BUNK OVERHEAD PIPE CHIN-UP & LEDGE PULL (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
        SendKeyTap(VK_SPACE);
        SendKeyTap('C'); // Hoist body into suspended ceiling tuck
        vrState.leftHapticPulse = 0.70f; // Iron pipe flex strain (75 Hz)
        vrState.rightHapticPulse = 0.70f;
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

    // Dark Athena EVA Micro-Thruster Jetpack Burst (ADR 0098)
    static DWORD s_lastJetpackBurstTime = 0;
    bool handsBehindHips = (leftLocal.z > -0.06f && rightLocal.z > -0.06f) &&
                          (fabsf(leftLocal.x) > 0.16f && fabsf(rightLocal.x) > 0.16f) &&
                          (leftLocal.y > -0.35f && leftLocal.y < -0.05f && rightLocal.y > -0.35f && rightLocal.y < -0.05f);
    bool bothTriggersJetpack = (vrState.leftController.trigger > 0.65f && vrState.rightController.trigger > 0.65f);
    if (handsBehindHips && bothTriggersJetpack && !isClimbingActive && (now - s_lastJetpackBurstTime > 300)) {
        s_lastJetpackBurstTime = now;
        VR_LOG_INFO("EVA Spacewalk: COLD-GAS MICRO-THRUSTERS FIRED (Lz: %.2f, Rz: %.2f)", leftLocal.z, rightLocal.z);
        SendKeyTap(VK_SPACE);
        SendKey('W', true);
        SendKey('W', false);
        vrState.leftHapticPulse = 0.65f; // Pneumatic gas jet thrust rumble (130 Hz)
        vrState.rightHapticPulse = 0.65f;
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
        // Guard Rifle Firearm Stripping & Mag Ejection Disarm (ADR 0063)
        bool dualRifleStrip = s_currentBlockEngaged &&
                              (rVel.z < -1.15f && lVel.z < -1.15f) &&
                              (vrState.rightController.trigger > 0.60f && vrState.leftController.trigger > 0.60f);

        bool isForwardParry = s_currentBlockEngaged &&
            ((rVel.z < -1.15f && rightLocal.z < -0.15f) || (lVel.z < -1.15f && leftLocal.z < -0.15f));

        // Inmate Shiv Parrying Riposte & Eye-Gouge Counter (ADR 0092)
        bool isEyeGougeRiposte = s_currentBlockEngaged &&
            (rVel.z < -1.10f && rVel.y > 1.05f && rightLocal.y > 0.04f);

        // Security Guard Riot Shield Baton Wedge Pry & Disarm (ADR 0111)
        bool bothAtShieldZone = (rightLocal.z > -0.42f && rightLocal.z < -0.16f && leftLocal.z > -0.42f && leftLocal.z < -0.16f) &&
                                (rightLocal.y > -0.20f && rightLocal.y < 0.20f && leftLocal.y > -0.20f && leftLocal.y < 0.20f);
        bool shieldGrippedAndWedged = (vrState.leftController.grip > 0.60f) &&
                                      (vrState.rightController.trigger > 0.50f || vrState.rightController.grip > 0.60f);
        bool opposingShieldWrench = (lVel.x < -0.90f || lVel.y < -0.80f) && (rVel.x > 0.90f || rVel.y > 0.80f);
        bool isShieldPryDisarm = bothAtShieldZone && shieldGrippedAndWedged && opposingShieldWrench;

        // Inmate Pit Brawl Pocket Ash Throw Riposte (ADR 0115)
        static DWORD s_lastPocketAshRiposteTime = 0;
        bool rightAtPocket = (rightLocal.y > -0.45f && rightLocal.y < -0.15f) &&
                             (fabsf(rightLocal.x) > 0.10f && fabsf(rightLocal.x) < 0.26f) &&
                             (vrState.rightController.grip > 0.50f);
        bool pocketAshFling = (rVel.y > 1.10f && rVel.z < -0.90f);
        bool isPocketAshRiposte = s_currentBlockEngaged && rightAtPocket && pocketAshFling;

        if (dualRifleStrip) {
            s_lastPunchTime = now;
            VR_LOG_INFO("Physical Counter: GUARD RIFLE STRIP & MAG EJECTION DISARM (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            SendKeyTap('R'); // Drop enemy magazine
            SendKeyTap('1'); // Equip stripped firearm
            vrState.rightHapticPulse = 0.95f; // Receiver wrenching shockwave (280 Hz)
            vrState.leftHapticPulse = 0.95f;
        } else if (isPocketAshRiposte && (now - s_lastPocketAshRiposteTime > 1600)) {
            s_lastPocketAshRiposteTime = now;
            s_lastPunchTime = now;
            VR_LOG_INFO("Physical Counter: POCKET ASH THROW BLIND & RIPOSTE (Ry: %.2f, Rz: %.2f)", rVel.y, rVel.z);
            SendKeyTap('E');
            SendKeyTap(VK_TAB);
            SendKey('W', true);
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            SendKey('W', false);
            vrState.rightHapticPulse = 0.85f; // Particulate scatter & strike snap (220 Hz)
        } else if (isShieldPryDisarm) {
            s_lastPunchTime = now;
            VR_LOG_INFO("Physical Counter: RIOT SHIELD WEDGE PRY & DISARM (Lx: %.2f, Rx: %.2f)", lVel.x, rVel.x);
            SendKeyTap('E');
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            SendKeyTap('1'); // Seize weapon slot
            vrState.leftHapticPulse = 0.95f; // Metallic shear crack (280 Hz)
            vrState.rightHapticPulse = 0.95f;
        } else if (isEyeGougeRiposte) {
            s_lastPunchTime = now;
            VR_LOG_INFO("Physical Counter: INMATE SHIV EYE-GOUGE RIPOSTE (Rz: %.2f, Ry: %.2f, Ey: %.2f)", rVel.z, rVel.y, rightLocal.y);
            SendKey('W', true);
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            SendKey('W', false);
            vrState.rightHapticPulse = 0.95f; // Perforation & orbital bone crunch (280 Hz)
        } else if (isForwardParry) {
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

            // Heavy Exoskeleton Power Fist Kinetic Shockwave Punch (ADR 0066)
            bool mechPowerPunch = (rVel.z < -1.60f) && (vrState.rightController.grip > 0.60f);

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

            // Inmate Mess Hall Tray Facial Batter (ADR 0060)
            bool leftTrayBatter = (leftLocal.z < -0.18f) && (lVel.z < -1.0f) && (vrState.leftController.grip > 0.50f);

            // Guard Stun Baton Heavy Two-Handed Defibrillator Slam (ADR 0077)
            bool twoHandedBatonSlam = s_twoHandedStabilizationActive &&
                                      (lVel.y < -1.30f && rVel.y < -1.30f) &&
                                      (vrState.rightController.trigger > 0.60f && vrState.leftController.trigger > 0.60f);

            // Mining Core Heavy Ore Sledgehammer Two-Handed Overhead Crush (ADR 0102)
            float handsDistSledge = sqrtf(
                powf(leftLocal.x - rightLocal.x, 2) +
                powf(leftLocal.y - rightLocal.y, 2) +
                powf(leftLocal.z - rightLocal.z, 2)
            );
            bool dualSledgeOverhead = (handsDistSledge < 0.40f) &&
                                      (leftLocal.y > 0.28f && rightLocal.y > 0.28f) &&
                                      (vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f) &&
                                      (lVel.y < -1.40f && rVel.y < -1.40f);

            // Tactical Stun Baton Overload Arc Wave Discharge (ADR 0118)
            static DWORD s_lastArcWaveTime = 0;
            static DWORD s_arcWaveChargeStart = 0;
            bool crossedBatonCharge = (handsDistSledge < 0.18f) &&
                                      (leftLocal.y > -0.25f && leftLocal.y < 0.10f && rightLocal.y > -0.25f && rightLocal.y < 0.10f) &&
                                      (vrState.leftController.grip > 0.70f && vrState.rightController.grip > 0.70f) &&
                                      (vrState.leftController.trigger > 0.65f && vrState.rightController.trigger > 0.65f);
            bool isDischargingArcWave = false;
            if (crossedBatonCharge) {
                if (s_arcWaveChargeStart == 0) s_arcWaveChargeStart = now;
                DWORD chargeTime = now - s_arcWaveChargeStart;
                float chargeRatio = (chargeTime < 400) ? (chargeTime / 400.0f) : 1.0f;
                vrState.leftHapticPulse = 0.30f + 0.45f * chargeRatio;
                vrState.rightHapticPulse = 0.30f + 0.45f * chargeRatio;
            } else {
                if (s_arcWaveChargeStart != 0 && (now - s_arcWaveChargeStart >= 250)) {
                    bool explosiveRadialThrust = (lVel.x < -1.05f && rVel.x > 1.05f) || (lVel.z < -1.10f && rVel.z < -1.10f);
                    if (explosiveRadialThrust && (now - s_lastArcWaveTime > 2500)) {
                        s_lastArcWaveTime = now;
                        isDischargingArcWave = true;
                    }
                }
                s_arcWaveChargeStart = 0;
            }

            bool crouchedForStealth = (s_isPhysicallyCrouched || s_isDeepProne);
            bool atThroatCorridor = (rightLocal.y > -0.12f && rightLocal.y < 0.16f) &&
                                    (rightLocal.z > -0.34f && rightLocal.z < -0.10f) &&
                                    (vrState.rightController.grip > 0.50f);
            bool carotidSlice = (fabsf(rVel.x) > 1.25f);
            bool stealthCarotidSlash = crouchedForStealth && atThroatCorridor && carotidSlice;

            // Inmate Shiv Rib-Cage Underhook Puncture & Lift (ADR 0122)
            static DWORD s_lastRibUnderhookTime = 0;
            bool lowStanceForUnderhook = (s_isPhysicallyCrouched || s_isDeepProne) || (rightLocal.y < -0.15f);
            bool reverseBladeGrip = (vrState.rightController.grip > 0.55f && vrState.rightController.trigger < 0.40f);
            bool underhookRisingThrust = (rVel.y > 1.15f && rVel.z < -0.90f);
            bool isRibUnderhook = lowStanceForUnderhook && reverseBladeGrip && underhookRisingThrust;

            if (isDischargingArcWave) {
                VR_LOG_INFO("Tactical Melee: STUN BATON OVERLOAD RADIAL ARC WAVE DISCHARGED (Lx: %.2f, Rx: %.2f)", lVel.x, rVel.x);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                vrState.leftHapticPulse = 0.98f;  // High-voltage electrostatic arc ring shockwave (300 Hz)
                vrState.rightHapticPulse = 0.98f;
            } else if (isRibUnderhook && (now - s_lastRibUnderhookTime > 1500)) {
                s_lastRibUnderhookTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE SHIV RIB-CAGE UNDERHOOK PUNCTURE & LIFT (Ry: %.2f, Rz: %.2f)", rVel.y, rVel.z);
                SendKey('W', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.95f; // Cartilage perforation & body lift rumble (280 Hz)
            } else if (dualSledgeOverhead) {
                VR_LOG_INFO("Physical Gesture: HEAVY ORE SLEDGEHAMMER OVERHEAD CRUSH (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.98f; // Catastrophic blunt fracture shockwave (80 Hz)
                vrState.leftHapticPulse = 0.98f;
            } else if (stealthCarotidSlash) {
                VR_LOG_INFO("Physical Stealth: CAROTID ARTERIAL JUGULAR SLASH (SpeedX: %.2f m/s, Ry: %.2f)", rVel.x, rightLocal.y);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.95f; // Arterial release incision & spray (260 Hz)
            } else if (twoHandedBatonSlam) {
                VR_LOG_INFO("Physical Gesture: TWO-HANDED STUN BATON DEFIBRILLATOR SLAM (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.98f; // Electro-kinetic shockwave blast (280 Hz)
                vrState.leftHapticPulse = 0.98f;
            } else if (mechPowerPunch) {
                VR_LOG_INFO("Physical Gesture: EXOSKELETON POWER FIST KINETIC SHOCKWAVE PUNCH (Rz: %.2f m/s)", rVel.z);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Piston shockwave blast (100 Hz)
                vrState.leftHapticPulse = 0.85f;
            } else if (droneEmpSnap) {
                VR_LOG_INFO("Physical Combat: SECURITY DRONE EMP OVERLOAD & CHASSIS SNAP (TorqueRelY: %.2f)", fabsf(lVel.y - rVel.y));
                SendKeyTap('E');
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                vrState.rightHapticPulse = 0.95f; // Mechanical spine crunch & EMP arc wave (280 Hz)
                vrState.leftHapticPulse = 0.95f;
            } else if (leftTrayBatter) {
                VR_LOG_INFO("Physical Gesture: INMATE MESS TRAY CONCUSSIVE BATTER (Lz: %.2f)", lVel.z);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.90f; // Sheet metal concussive clatter (160 Hz)
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

        // Cybernetic Drone HUD Battery Power Cell Hot-Swap Ejection (ADR 0082)
        bool cellEjectFlick = (reloadHandDist < 0.18f && vrState.leftController.grip > 0.50f && lVel.y < -0.70f);
        if (cellEjectFlick) {
            s_lastReloadGestureTime = now;
            VR_LOG_INFO("Tactical Weapon: POWER CELL HOT-SWAP EJECTED (Vy: %.2f m/s, Dist: %.2fm)", lVel.y, reloadHandDist);
            SendKeyTap('R');
            vrState.leftHapticPulse = 0.75f; // Magnetic cell disconnect snap (140 Hz)
            vrState.rightHapticPulse = 0.50f;
        } else if (offHandAtReceiver || handAtWaistPouch) {
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

    // 6a-2. Inmate Toothbrush Shank Whittling & Rapid Friction Sharpening (ADR 0068)
    static DWORD s_lastWhittleStrokeTime = 0;
    static int s_whittleStrokeCount = 0;
    bool crouchedForWhittle = (s_isPhysicallyCrouched || s_isDeepProne);
    bool dominantNearFloor = (rightLocal.y < -0.45f && rightLocal.z > -0.45f && rightLocal.z < -0.15f);
    bool grippingShank = (vrState.rightController.grip > 0.50f);
    bool fastFrictionStroke = (fabsf(rVel.x) > 0.65f);
    if (crouchedForWhittle && dominantNearFloor && grippingShank && fastFrictionStroke && !s_physicalBlockEngaged) {
        if (now - s_lastWhittleStrokeTime > 180) {
            s_lastWhittleStrokeTime = now;
            s_whittleStrokeCount++;
            VR_LOG_INFO("Physical Gesture: TOOTHBRUSH SHANK WHITTLING STROKE #%d (SpeedX: %.2f m/s)", s_whittleStrokeCount, rVel.x);
            vrState.rightHapticPulse = 0.50f; // Abrasive concrete grinding pulse (200 Hz)
            if (s_whittleStrokeCount >= 5) {
                s_whittleStrokeCount = 0;
                VR_LOG_INFO("Physical Gesture: IMPROVISED SHIV RAZOR TIP HONED / EQUIPPED");
                SendKeyTap('E');
                SendKeyTap('3'); // Equip shank slot
                vrState.rightHapticPulse = 0.70f; // Hone completion resonance (260 Hz)
            }
        }
    }

    // 6a-3. Inmate Mess Hall Contraband Spork Whittling & Tine Sharpening (ADR 0080)
    static DWORD s_lastSporkStrokeTime = 0;
    static int s_sporkStrokeCount = 0;
    bool sporkTableHeight = (rightLocal.y > -0.55f && rightLocal.y < -0.25f) &&
                            (rightLocal.z > -0.42f && rightLocal.z < -0.16f);
    bool fastSporkStroke = (fabsf(rVel.x) > 0.60f);
    if (crouchedForWhittle && sporkTableHeight && grippingShank && fastSporkStroke && !s_physicalBlockEngaged) {
        if (now - s_lastSporkStrokeTime > 160) {
            s_lastSporkStrokeTime = now;
            s_sporkStrokeCount++;
            VR_LOG_INFO("Physical Gesture: MESS SPORK TINE NOTCHING STROKE #%d (SpeedX: %.2f m/s)", s_sporkStrokeCount, rVel.x);
            vrState.rightHapticPulse = 0.45f; // Serrated rasp pulse (190 Hz)
            if (s_sporkStrokeCount >= 4) {
                s_sporkStrokeCount = 0;
                VR_LOG_INFO("Physical Gesture: SERRATED CONTRABAND PUNCTURE SPORK CRAFTED / EQUIPPED");
                SendKeyTap('E');
                SendKeyTap('3'); // Equip shank/shiv slot
                vrState.rightHapticPulse = 0.70f; // Hone ping resonance (240 Hz)
            }
        }
    }

    // 6a-4. Security Camera Thermal Decoy Flare Igniting Gesture (ADR 0089)
    static DWORD s_lastFlareIgniteTime = 0;
    bool flareInOffHand = (leftLocal.z > -0.35f && leftLocal.z < -0.15f) && (vrState.leftController.grip > 0.55f);
    bool strikerAtCap = (reloadHandDist < 0.16f) && (vrState.rightController.grip > 0.55f);
    float shearVelY = fabsf(lVel.y - rVel.y);
    float shearVelX = fabsf(lVel.x - rVel.x);
    if (flareInOffHand && strikerAtCap && (shearVelY > 0.80f || shearVelX > 0.80f) && !s_physicalBlockEngaged && (now - s_lastFlareIgniteTime > 1800)) {
        s_lastFlareIgniteTime = now;
        VR_LOG_INFO("Tactical Decoy: THERMAL MAGNESIUM FLARE IGNITED (ShearVelY: %.2f, ShearVelX: %.2f)", shearVelY, shearVelX);
        SendKeyTap('4'); // Equip flare / throwable slot
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        vrState.leftHapticPulse = 0.85f;  // Sulfur percussion strike snap (260 Hz)
        vrState.rightHapticPulse = 0.85f;
    }

    // 6a-5. Medical Syringe Sedative Self-Injection & Bio-Adrenaline Rush (ADR 0087)
    static DWORD s_lastSyringeInjectTime = 0;
    bool dominantAtOffArm = (rightLocal.x > -0.24f && rightLocal.x < -0.06f) &&
                            (rightLocal.y > -0.18f && rightLocal.y < 0.12f) &&
                            (rightLocal.z > -0.25f && rightLocal.z < -0.05f);
    bool syringePlungerPressed = (vrState.rightController.grip > 0.50f) && (vrState.rightController.trigger > 0.65f);
    if (dominantAtOffArm && syringePlungerPressed && !s_physicalBlockEngaged && (now - s_lastSyringeInjectTime > 1500)) {
        s_lastSyringeInjectTime = now;
        VR_LOG_INFO("Medical Restoration: PNEUMATIC BIO-ADRENALINE SYRINGE INJECTED (Rx: %.2f, Ry: %.2f, Rz: %.2f)", rightLocal.x, rightLocal.y, rightLocal.z);
        SendKeyTap('E');
        SendKeyTap('Q'); // Bio-adrenaline / stimulant surge
        vrState.rightHapticPulse = 0.85f; // Needle plunge & adrenaline rush pulse (100 Hz)
        vrState.leftHapticPulse = 0.65f;  // Arm tissue injection feedback
    }

    // 6a-6. Inmate Pit Brawl Dirt / Ash Pocket Concealment (ADR 0096)
    static DWORD s_lastGritScoopTime = 0;
    static bool s_hasScoopedGrit = false;
    bool crouchedNearGround = (s_isPhysicallyCrouched || s_isDeepProne) && (rightLocal.y < -0.45f);
    bool gritFloorScoop = crouchedNearGround && (vrState.rightController.grip > 0.50f) && (rVel.z > 0.35f);
    if (gritFloorScoop && !s_hasScoopedGrit && (now - s_lastGritScoopTime > 1500)) {
        s_hasScoopedGrit = true;
        s_lastGritScoopTime = now;
        VR_LOG_INFO("Physical Stealth: PIT BRAWL DIRT / ASH SCOOPED FROM FLOOR (Ry: %.2f, Rz: %.2f)", rightLocal.y, rVel.z);
        vrState.rightHapticPulse = 0.35f; // Abrasive grit sifting pulse (140 Hz)
    }
    if (s_hasScoopedGrit && (rightLocal.y > -0.38f && rightLocal.y < -0.15f && fabsf(rightLocal.x) > 0.12f)) {
        s_hasScoopedGrit = false;
        VR_LOG_INFO("Physical Stealth: PIT BRAWL GRIT CONCEALED IN WAIST POCKET");
        SendKeyTap('E');
        SendKeyTap(VK_TAB);
        vrState.rightHapticPulse = 0.45f; // Cloth pocket rustle confirmation (180 Hz)
    }

    // 6a-7. Security Guard Taser Wire Snip & Circuit Disarm (ADR 0101)
    static DWORD s_lastTaserSnipTime = 0;
    bool handAtChestCables = (fabsf(rightLocal.x) < 0.15f) &&
                             (rightLocal.y > -0.18f && rightLocal.y < 0.10f) &&
                             (rightLocal.z > -0.24f && rightLocal.z < -0.05f) &&
                             (vrState.rightController.grip > 0.50f);
    bool wireShearSlice = (fabsf(rVel.x) > 0.90f);
    if (handAtChestCables && wireShearSlice && !s_physicalBlockEngaged && (now - s_lastTaserSnipTime > 1200)) {
        s_lastTaserSnipTime = now;
        VR_LOG_INFO("Tactical Defense: GUARD TASER WIRE SNIPPED / CIRCUIT GROUNDED (Vx: %.2f m/s)", rVel.x);
        SendKeyTap('E');
        SendKeyTap('3'); // Ground discharge into shiv blade
        vrState.rightHapticPulse = 0.75f; // Dielectric wire shear snap (220 Hz)
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

    // Security Guard DNA Biometric Weapon Grip Shock Suppression (ADR 0083)
    static DWORD s_lastDnaSuppressionTime = 0;
    bool insulatedTapeWrap = (twoHandDist < 0.15f) && (vrState.leftController.grip > 0.60f);
    if (insulatedTapeWrap && !s_physicalBlockEngaged && (now - s_lastDnaSuppressionTime > 1500)) {
        s_lastDnaSuppressionTime = now;
        VR_LOG_INFO("Tactical Firearm: GUARD DNA BIOMETRIC CIRCUIT SUPPRESSED WITH INSULATED GAUNTLET (Dist: %.2fm)", twoHandDist);
        SendKeyTap('E'); // Suppress/bypass DNA lock circuit
        vrState.leftHapticPulse = 0.40f; // Neutralized low hum (80 Hz)
        vrState.rightHapticPulse = 0.40f;
    }

    // Tactical Laser Sighting Module Pressure Pad Foregrip Tap (ADR 0091)
    static DWORD s_lastLaserToggleTime = 0;
    bool foregripLaserPad = (vrState.leftController.trigger > 0.50f) || ((vrState.leftController.buttons & RIDDICK_VR_BTN_STICK_LEFT) != 0);
    if (s_twoHandedStabilizationActive && foregripLaserPad && (now - s_lastLaserToggleTime > 350)) {
        s_lastLaserToggleTime = now;
        VR_LOG_INFO("Tactical Firearm: FOREGRIP PRESSURE PAD LASER SIGHT TOGGLED");
        SendKeyTap('F');
        vrState.leftHapticPulse = 0.45f; // Crisp micro-switch click (180 Hz)
    }

    // Dark Athena Drone Optical Sensor Scramble Laser Strike (ADR 0103)
    static DWORD s_laserScrambleDwellStart = 0;
    static DWORD s_lastLaserScrambleTime = 0;
    bool droneSensorAimLock = (rightLocal.z > -0.45f && rightLocal.z < -0.18f) &&
                              (rightLocal.y > -0.10f && rightLocal.y < 0.30f) &&
                              (fabsf(rSpeed) < 0.20f);
    bool laserEngaged = (vrState.leftController.trigger > 0.50f);
    if (s_twoHandedStabilizationActive && laserEngaged && droneSensorAimLock) {
        if (s_laserScrambleDwellStart == 0) {
            s_laserScrambleDwellStart = now;
        } else if ((now - s_laserScrambleDwellStart > 500) && (now - s_lastLaserScrambleTime > 1600)) {
            s_lastLaserScrambleTime = now;
            s_laserScrambleDwellStart = 0;
            VR_LOG_INFO("Tactical Firearm: DRONE OPTICAL SENSOR SCRAMBLED VIA COHERENT LASER (Rz: %.2f, Ry: %.2f)", rightLocal.z, rightLocal.y);
            SendKeyTap('F');
            SendKeyTap('E');
            vrState.leftHapticPulse = 0.65f; // Optic overload pop (240 Hz)
            vrState.rightHapticPulse = 0.65f;
        }
    } else {
        s_laserScrambleDwellStart = 0;
    }

    // Apply exponential smoothing / spherical interpolation on weapon aim pose when two-handed
    if (vrState.rightController.aimPose.isValid) {
        if (s_twoHandedStabilizationActive) {
            if (!s_hasStabilizedPose) {
                s_stabilizedAimPose = vrState.rightController.aimPose;
                s_hasStabilizedPose = true;
            } else {
                float smoothAlpha = isHarriesStance ? 0.22f : 0.35f;

                // Cybernetic Drone HUD Decoupled Sight & Reticle Drift Dampening (ADR 0062)
                bool isDroneAiming = (vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
                if (isDroneAiming) {
                    smoothAlpha = 0.28f; // Heavy cybernetic servo lag & drift dampening
                }

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

    // Laser Tripwire Spatial Shield Deflection (ADR 0059)
    static DWORD s_lastLaserDeflectTime = 0;
    if (s_twoHandedStabilizationActive) {
        bool shieldHeldForward = (rightLocal.z > -0.35f && rightLocal.z < -0.15f) && (fabsf(rightLocal.y) < 0.20f);
        if (shieldHeldForward && (now - s_lastLaserDeflectTime > 150)) {
            s_lastLaserDeflectTime = now;
            vrState.rightHapticPulse = 0.30f; // Laser scatter refraction micro-vibration (220 Hz)
            vrState.leftHapticPulse = 0.30f;
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

    // Heavy Riot Shotgun Breaching Hinge Blast & Kick (ADR 0095)
    static DWORD s_lastShotgunBreachTime = 0;
    bool pointBlankLock = (rightLocal.z > -0.38f && rightLocal.z < -0.16f) && (fabsf(rightLocal.y) < 0.15f);
    bool breachForwardThrust = (rVel.z < -1.20f && lVel.z < -1.20f);
    bool triggerBreachPull = (vrState.rightController.trigger > 0.80f);
    if (s_twoHandedStabilizationActive && pointBlankLock && breachForwardThrust && triggerBreachPull && (now - s_lastShotgunBreachTime > 1800)) {
        s_lastShotgunBreachTime = now;
        VR_LOG_INFO("Physical Gunplay: SHOTGUN BREACHING BLAST & DOOR KICK (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        SendKeyTap('W');
        SendKeyTap(VK_SPACE);
        vrState.rightHapticPulse = 0.98f; // Point-blank concussive muzzle blast (90 Hz)
        vrState.leftHapticPulse = 0.98f;
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

    // Heavy Mining Pneumatic Jackhammer Dual-Handle Ground Chisel (ADR 0113)
    static DWORD s_lastGroundChiselTime = 0;
    float dxJackhammer = leftLocal.x - rightLocal.x;
    float dyJackhammer = leftLocal.y - rightLocal.y;
    float dzJackhammer = leftLocal.z - rightLocal.z;
    float distJackhammer = sqrtf(dxJackhammer * dxJackhammer + dyJackhammer * dyJackhammer + dzJackhammer * dzJackhammer);

    bool dualHandleGrip = (distJackhammer >= 0.22f && distJackhammer <= 0.45f) &&
                          (leftLocal.y > -0.45f && leftLocal.y < -0.05f && rightLocal.y > -0.45f && rightLocal.y < -0.05f) &&
                          (vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
    bool violentDownwardChiselPlunge = (lVel.y < -1.15f && rVel.y < -1.15f);
    if (dualHandleGrip && violentDownwardChiselPlunge && (now - s_lastGroundChiselTime > 2000)) {
        s_lastGroundChiselTime = now;
        VR_LOG_INFO("Environmental Gesture: MINING PNEUMATIC JACKHAMMER GROUND CHISEL PLUNGE (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
        SendKeyTap('S');
        SendKeyTap(VK_SPACE);
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        vrState.leftHapticPulse = 0.98f;  // Seismic percussive hammer strike (50 Hz)
        vrState.rightHapticPulse = 0.98f;
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

            // Cryo-Chamber Defrost Thermal Cycle Override Keypad Punch (ADR 0112)
            static DWORD s_lastDefrostTapTime = 0;
            static int s_defrostTapCount = 0;
            bool handAtDefrostKeypad = ((rightLocal.z > -0.38f && rightLocal.z < -0.15f && rightLocal.y > -0.15f && rightLocal.y < 0.20f && fabsf(rightLocal.x) < 0.22f && vrState.rightController.trigger < 0.20f && vrState.rightController.grip < 0.35f) ||
                                        (leftLocal.z > -0.38f && leftLocal.z < -0.15f && leftLocal.y > -0.15f && leftLocal.y < 0.20f && fabsf(leftLocal.x) < 0.22f && vrState.leftController.trigger < 0.20f && vrState.leftController.grip < 0.35f));
            bool defrostPoke = (rVel.z < -0.32f || lVel.z < -0.32f);
            if (handAtDefrostKeypad && defrostPoke && (now - s_lastDefrostTapTime > 220)) {
                s_lastDefrostTapTime = now;
                s_defrostTapCount++;
                VR_LOG_INFO("Environmental Gesture: CRYO-CHAMBER DEFROST KEYPAD TAP #%d", s_defrostTapCount);
                if (rVel.z < -0.32f) vrState.rightHapticPulse = 0.45f; // Membrane click (260 Hz)
                if (lVel.z < -0.32f) vrState.leftHapticPulse = 0.45f;
                if (s_defrostTapCount >= 3) {
                    s_defrostTapCount = 0;
                    VR_LOG_INFO("Environmental Gesture: CRYO DEFROST CYCLE INITIATED / THERMAL BYPASS CONFIRMED");
                    SendKeyTap('E');
                    SendKeyTap(VK_RETURN);
                    SendKeyTap('Q');
                    vrState.rightHapticPulse = 0.70f; // Confirmation chime (180 Hz)
                    vrState.leftHapticPulse = 0.70f;
                }
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

            // Bio-Containment Decontamination Airlock Lever (ADR 0064)
            bool rightAtDeconLever = (rightLocal.z > -0.42f && rightLocal.z < -0.18f) &&
                                     (rightLocal.y > 0.05f && rightLocal.y < 0.35f) &&
                                     (vrState.rightController.grip > 0.50f) &&
                                     (rVel.y < -0.60f);
            if (rightAtDeconLever && (now - s_lastEnvInteractTime > 800)) {
                s_lastEnvInteractTime = now;
                VR_LOG_INFO("Environmental Gesture: BIO-CONTAINMENT DECONTAMINATION LEVER PULLED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                vrState.rightHapticPulse = 0.80f; // Pressurized decontamination air blast whoosh (140 Hz)
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

            // Inmate Mess Hall Tray Equip (ADR 0060)
            static DWORD s_lastTrayEquipTime = 0;
            bool leftNearMessTable = (leftLocal.y > -0.55f && leftLocal.y < -0.25f) &&
                                     (leftLocal.z > -0.40f && leftLocal.z < -0.15f) &&
                                     (vrState.leftController.grip > 0.50f);
            if (leftNearMessTable && (now - s_lastTrayEquipTime > 1000)) {
                s_lastTrayEquipTime = now;
                VR_LOG_INFO("Environmental Gesture: INMATE MESS TRAY EQUIPPED FROM TABLE (Y: %.2fm)", leftLocal.y);
                SendKeyTap('E');
                SendKeyTap('1'); // Equip improvised defensive slot
                vrState.leftHapticPulse = 0.40f; // Stamped sheet metal clang (180 Hz)
            }

            // Hydraulic Pressure Blast Valve Steam Jet Venting (ADR 0061)
            static DWORD s_lastSteamJetTime = 0;
            bool rightAtBlastValve = (rightLocal.z > -0.40f && rightLocal.z < -0.18f) &&
                                     (rightLocal.y > -0.20f && rightLocal.y < 0.20f);
            bool blastStrike = (rVel.z < -1.10f || rVel.y < -0.80f);
            if (rightAtBlastValve && blastStrike && (now - s_lastSteamJetTime > 1200)) {
                s_lastSteamJetTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDRAULIC PRESSURE BLAST VALVE POPPED / STEAM JET RELEASED");
                SendKeyTap('E');
                SendKeyTap('Q'); // Thermal diversion signal
                vrState.rightHapticPulse = 0.75f; // Pressurized jet stream vibration (130 Hz)
            }

            // Overhead Vent Hatch Dual-Palm Upward Shove (ADR 0067)
            bool hmdLookingUp = (s_lastHmdPitch > 0.45f);
            bool bothOverheadHatch = (leftLocal.y > 0.25f && rightLocal.y > 0.25f) &&
                                     (leftLocal.z > -0.25f && leftLocal.z < 0.10f && rightLocal.z > -0.25f && rightLocal.z < 0.10f);
            bool bothPalmsUp = (vrState.leftController.grip < 0.60f && vrState.rightController.grip < 0.60f);
            bool bothShovingUp = (lVel.y > 0.85f && rVel.y > 0.85f);
            if (hmdLookingUp && bothOverheadHatch && bothPalmsUp && bothShovingUp && (now - s_lastEnvInteractTime > 800)) {
                s_lastEnvInteractTime = now;
                VR_LOG_INFO("Environmental Gesture: OVERHEAD VENT HATCH SHOVED OPEN (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                vrState.leftHapticPulse = 0.80f; // Spring-loaded latch pop & metal resonance (160 Hz)
                vrState.rightHapticPulse = 0.80f;
            }

            // Cryo-Chamber Emergency Oxygen Mask Quick-Don Stance (ADR 0069)
            static DWORD s_maskHoldStartTime = 0;
            static DWORD s_lastMaskEquipTime = 0;
            bool handAtMouth = (leftLocal.z > -0.20f && leftLocal.z < -0.05f) &&
                               (leftLocal.y > -0.18f && leftLocal.y < 0.06f) &&
                               (fabsf(leftLocal.x) < 0.12f);
            bool grippingMask = (vrState.leftController.grip > 0.50f);
            if (handAtMouth && grippingMask) {
                if (s_maskHoldStartTime == 0) {
                    s_maskHoldStartTime = now;
                } else if ((now - s_maskHoldStartTime >= 150) && (now - s_lastMaskEquipTime > 1500)) {
                    s_lastMaskEquipTime = now;
                    VR_LOG_INFO("Environmental Gear: EMERGENCY REBREATHER / OXYGEN MASK CLAMPED TO FACE (Y: %.2fm)", leftLocal.y);
                    SendKeyTap('E'); // Seal environmental rebreather
                    vrState.leftHapticPulse = 0.65f; // Pneumatic seal whoosh & inhalation pulse (110 Hz)
                }
            } else {
                s_maskHoldStartTime = 0;
            }

            // Automated Turret Manual Traversal Handwheel Slew (ADR 0070)
            static DWORD s_lastTurretGearRumble = 0;
            static bool s_turretSteeringActive = false;
            bool bothAtTurretStation = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && rightLocal.z > -0.42f && rightLocal.z < -0.16f) &&
                                       (leftLocal.y > -0.30f && leftLocal.y < 0.10f && rightLocal.y > -0.30f && rightLocal.y < 0.10f);
            bool dualGrippingTurret = (vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
            float spadeGripDist = fabsf(leftLocal.x - rightLocal.x);
            bool validSpadeWidth = (spadeGripDist >= 0.25f && spadeGripDist <= 0.48f);

            if (bothAtTurretStation && dualGrippingTurret && validSpadeWidth) {
                s_turretSteeringActive = true;
                float dZ = leftLocal.z - rightLocal.z;
                float avgY = 0.5f * (leftLocal.y + rightLocal.y);

                // Azimuth / Yaw Slew (Left/Right)
                if (dZ > 0.08f) {
                    SendKey('D', true);
                    SendKey('A', false);
                } else if (dZ < -0.08f) {
                    SendKey('A', true);
                    SendKey('D', false);
                } else {
                    SendKey('A', false);
                    SendKey('D', false);
                }

                // Elevation / Pitch Slew (Up/Down)
                if (avgY > 0.05f) {
                    SendKey(VK_UP, true);
                    SendKey(VK_DOWN, false);
                } else if (avgY < -0.18f) {
                    SendKey(VK_DOWN, true);
                    SendKey(VK_UP, false);
                } else {
                    SendKey(VK_UP, false);
                    SendKey(VK_DOWN, false);
                }

                // Turret Hydraulic Gear Train Haptics
                bool isMovingTurret = (fabsf(dZ) > 0.08f) || (avgY > 0.05f || avgY < -0.18f);
                if (isMovingTurret && (now - s_lastTurretGearRumble > 160)) {
                    s_lastTurretGearRumble = now;
                    vrState.leftHapticPulse = 0.45f; // Deep gear train meshing rumble (55 Hz)
                    vrState.rightHapticPulse = 0.45f;
                }
            } else if (s_turretSteeringActive) {
                s_turretSteeringActive = false;
                SendKey('A', false);
                SendKey('D', false);
                SendKey(VK_UP, false);
                SendKey(VK_DOWN, false);
            }

            // Cargo Bay Container Hydraulic Lock Latch Lever (ADR 0072)
            static DWORD s_lastContainerLatchTime = 0;
            bool rightAtContainer = (rightLocal.z > -0.42f && rightLocal.z < -0.16f) &&
                                    (rightLocal.y > -0.25f && rightLocal.y < 0.15f) &&
                                    (vrState.rightController.grip > 0.50f);
            bool containerLeverThrow = (rVel.y > 0.70f || fabsf(rVel.x) > 0.70f);
            if (rightAtContainer && containerLeverThrow && (now - s_lastContainerLatchTime > 900)) {
                s_lastContainerLatchTime = now;
                VR_LOG_INFO("Environmental Gesture: CARGO CONTAINER HYDRAULIC LOCK UNLATCHED (Vy: %.2f, Vx: %.2f)", rVel.y, rVel.x);
                SendKeyTap('E');
                vrState.rightHapticPulse = 0.85f; // Heavy hydraulic seal snap & mechanical clunk (100 Hz)
            }

            // Subterranean Cryo-Nitrogen Coolant Valve Venting Diversion (ADR 0074)
            static DWORD s_lastCryoCoolantVentTime = 0;
            bool rightAtCryoValve = (rightLocal.z > -0.42f && rightLocal.z < -0.16f) &&
                                    (rightLocal.y > -0.20f && rightLocal.y < 0.25f) &&
                                    (vrState.rightController.grip > 0.50f);
            bool cryoStrike = (rSpeed > 1.05f || fabsf(rVel.x) > 0.80f);
            if (rightAtCryoValve && cryoStrike && (now - s_lastCryoCoolantVentTime > 1200)) {
                s_lastCryoCoolantVentTime = now;
                VR_LOG_INFO("Environmental Gesture: CRYO-NITROGEN COOLANT VALVE POPPED / SUB-ZERO PLUME RELEASED");
                SendKeyTap('E');
                SendKeyTap('Q'); // Thermal diversion signal
                vrState.rightHapticPulse = 0.85f; // Crystalline ice fracture crack (260 Hz)
            }

            // Subterranean Mining Cart Rail Track Switch Wrench Pull (ADR 0076)
            static DWORD s_lastTrackSwitchTime = 0;
            bool rightAtTrackSwitch = (rightLocal.z > -0.42f && rightLocal.z < -0.16f) &&
                                      (rightLocal.y > -0.55f && rightLocal.y < -0.15f) &&
                                      (vrState.rightController.grip > 0.50f);
            bool trackSwitchThrow = (fabsf(rVel.x) > 0.70f);
            if (rightAtTrackSwitch && trackSwitchThrow && (now - s_lastTrackSwitchTime > 850)) {
                s_lastTrackSwitchTime = now;
                VR_LOG_INFO("Environmental Gesture: MINING RAIL TRACK SWITCH WRENCH THROWN (Vx: %.2f)", rVel.x);
                SendKeyTap('E');
                vrState.rightHapticPulse = 0.85f; // Heavy iron rail clank & ground shudder (70 Hz)
            }

            // Subterranean Mining Rail Turntable Manual Push Lever (ADR 0108)
            static DWORD s_lastTurntablePushTime = 0;
            static int s_turntablePushCount = 0;
            bool handAtTurntableLever = (rightLocal.z > -0.45f && rightLocal.z < -0.16f) &&
                                        (rightLocal.y > -0.40f && rightLocal.y < -0.05f) &&
                                        (vrState.rightController.grip > 0.55f);
            bool turntableTangentialPush = (fabsf(rVel.x) > 0.50f || fabsf(rVel.z) > 0.50f);
            if (handAtTurntableLever && turntableTangentialPush && (now - s_lastTurntablePushTime > 250)) {
                s_lastTurntablePushTime = now;
                s_turntablePushCount++;
                VR_LOG_INFO("Environmental Gesture: MINING RAIL TURNTABLE ROTATION STRIDE #%d (Speed: %.2f m/s)", s_turntablePushCount, rSpeed);
                vrState.rightHapticPulse = 0.65f; // Heavy ball-bearing grinding rumble (55 Hz)
                if (s_turntablePushCount >= 3) {
                    s_turntablePushCount = 0;
                    VR_LOG_INFO("Environmental Gesture: MINING RAIL TURNTABLE LOCKED INTO QUADRANT DETENT");
                    SendKeyTap('E');
                    SendKeyTap(VK_SPACE);
                    SendKeyTap(VK_TAB);
                    vrState.rightHapticPulse = 0.90f; // Concussive mechanical detent lock (80 Hz)
                }
            }

            // Subterranean Mining Rail Track Derailer Wedge Strike (ADR 0120)
            static DWORD s_lastDerailerStrikeTime = 0;
            bool leftAtRailWedge = (s_isPhysicallyCrouched || s_isDeepProne) &&
                                   (leftLocal.z > -0.45f && leftLocal.z < -0.16f && leftLocal.y < -0.45f) &&
                                   (vrState.leftController.grip > 0.55f);
            bool rightHammerStrikeDown = (rVel.y < -1.10f);
            if (leftAtRailWedge && rightHammerStrikeDown && (now - s_lastDerailerStrikeTime > 1800)) {
                s_lastDerailerStrikeTime = now;
                VR_LOG_INFO("Environmental Gesture: MINING RAIL DERAILER WEDGE STRUCK & SET (Ry: %.2f m/s)", rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                vrState.rightHapticPulse = 0.95f; // Anvil crack shockwave (260 Hz)
                vrState.leftHapticPulse = 0.60f;  // Rail acoustic ring (85 Hz)
            }

            // Dark Athena EVA Airlock Manual Handwheel Decompression Dogging (ADR 0078)
            static DWORD s_lastAirlockCrankTime = 0;
            static int s_airlockCrankCount = 0;
            bool bothAtAirlockWheel = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && rightLocal.z > -0.42f && rightLocal.z < -0.16f) &&
                                      (leftLocal.y > -0.20f && leftLocal.y < 0.25f && rightLocal.y > -0.20f && rightLocal.y < 0.25f) &&
                                      (vrState.leftController.grip > 0.50f && vrState.rightController.grip > 0.50f);
            float dualWheelSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y);
            if (bothAtAirlockWheel && dualWheelSpeed > 0.55f && (now - s_lastAirlockCrankTime > 220)) {
                s_lastAirlockCrankTime = now;
                s_airlockCrankCount++;
                VR_LOG_INFO("Environmental Gesture: EVA AIRLOCK HANDWHEEL DOGGING REVOLUTION #%d (Speed: %.2f m/s)", s_airlockCrankCount, dualWheelSpeed);
                vrState.leftHapticPulse = 0.50f;
                vrState.rightHapticPulse = 0.50f; // Ratchet tooth click (160 Hz)
                if (s_airlockCrankCount >= 3) {
                    s_airlockCrankCount = 0;
                    VR_LOG_INFO("Environmental Gesture: EVA AIRLOCK BULKHEAD DOGGED SHUT / HERMETIC REPRESSURIZATION TRIGGERED");
                    SendKeyTap('E');
                    vrState.leftHapticPulse = 0.85f; // Hermetic clamp thud & seal hiss (100 Hz)
                    vrState.rightHapticPulse = 0.85f;
                }
            }

            // Dark Athena EVA Solar Array Orientation Manual Hand Crank (ADR 0114)
            static DWORD s_lastSolarArrayCrankTime = 0;
            static int s_solarArrayCrankCount = 0;
            bool bothAtSolarTruss = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && rightLocal.z > -0.42f && rightLocal.z < -0.16f) &&
                                    (leftLocal.y > -0.15f && leftLocal.y < 0.25f && rightLocal.y > -0.15f && rightLocal.y < 0.25f) &&
                                    (vrState.leftController.grip > 0.50f && vrState.rightController.grip > 0.50f);
            float solarCrankSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y);
            if (bothAtSolarTruss && solarCrankSpeed > 0.50f && (now - s_lastSolarArrayCrankTime > 240)) {
                s_lastSolarArrayCrankTime = now;
                s_solarArrayCrankCount++;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR ARRAY SPINDLE REVOLUTION #%d (Speed: %.2f m/s)", s_solarArrayCrankCount, solarCrankSpeed);
                vrState.leftHapticPulse = 0.50f;
                vrState.rightHapticPulse = 0.50f; // Planetary gear teeth click (150 Hz)
                if (s_solarArrayCrankCount >= 3) {
                    s_solarArrayCrankCount = 0;
                    VR_LOG_INFO("Environmental Gesture: SOLAR ARRAY ORIENTATION ACHIEVED / AUXILIARY POWER RESTORED");
                    SendKeyTap('E');
                    SendKeyTap(VK_RETURN);
                    SendKeyTap('W');
                    vrState.leftHapticPulse = 0.85f;  // Electromagnetic solar collector lock hum (90 Hz)
                    vrState.rightHapticPulse = 0.85f;
                }
            }

            // Atmospheric Pressure Equalization Hatch Bleed Petcock Valve (ADR 0079)
            static DWORD s_lastPetcockTwistTime = 0;
            bool rightAtPetcock = (rightLocal.z > -0.40f && rightLocal.z < -0.15f) &&
                                  (rightLocal.y > -0.10f && rightLocal.y < 0.20f) &&
                                  (vrState.rightController.grip > 0.50f);
            bool petcockTwist = (fabsf(rVel.x) > 0.45f || fabsf(rVel.y) > 0.45f);
            if (rightAtPetcock && petcockTwist && (now - s_lastPetcockTwistTime > 900)) {
                s_lastPetcockTwistTime = now;
                VR_LOG_INFO("Environmental Gesture: ATMOSPHERIC BLEED PETCOCK VALVE TWISTED (Vx: %.2f)", rVel.x);
                SendKeyTap('E');
                vrState.rightHapticPulse = 0.55f; // Piercing pressure hiss & needle valve rattle (220 Hz)
            }

            // Security Turret Optical Sensor Spray Paint / Smear Blind (ADR 0081)
            static DWORD s_lastTurretBlindTime = 0;
            bool handAtTurretSensor = ((rightLocal.z > -0.35f && rightLocal.z < -0.15f && rightLocal.y > 0.10f && rightLocal.y < 0.40f && vrState.rightController.grip > 0.50f) ||
                                       (leftLocal.z > -0.35f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f && vrState.leftController.grip > 0.50f));
            bool sensorSmearSweep = (fabsf(rVel.x) > 0.50f || fabsf(lVel.x) > 0.50f);
            if (handAtTurretSensor && sensorSmearSweep && (now - s_lastTurretBlindTime > 1200)) {
                s_lastTurretBlindTime = now;
                VR_LOG_INFO("Environmental Stealth: TURRET OPTICAL SENSOR DOME SPRAY-PAINTED / COATED");
                SendKeyTap('E');
                SendKeyTap('Q'); // Optical suppression signal
                if (vrState.rightController.grip > 0.50f) vrState.rightHapticPulse = 0.50f; // Aerosol propellant hiss (160 Hz)
                if (vrState.leftController.grip > 0.50f) vrState.leftHapticPulse = 0.50f;
            }

            // Subterranean Ventilation Axial Fan Blade Jamming Bar (ADR 0084)
            static DWORD s_lastFanJamTime = 0;
            bool bothAtFanRotor = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && rightLocal.z > -0.42f && rightLocal.z < -0.16f) &&
                                  (leftLocal.y > -0.15f && leftLocal.y < 0.25f && rightLocal.y > -0.15f && rightLocal.y < 0.25f) &&
                                  (vrState.leftController.grip > 0.50f && vrState.rightController.grip > 0.50f);
            bool fanJamThrust = (lVel.z < -1.10f || rVel.z < -1.10f);
            if (bothAtFanRotor && fanJamThrust && (now - s_lastFanJamTime > 1200)) {
                s_lastFanJamTime = now;
                VR_LOG_INFO("Environmental Gesture: VENTILATION AXIAL FAN BLADES JAMMED (Lz: %.2f, Rz: %.2f)", lVel.z, rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                vrState.leftHapticPulse = 0.95f; // Shearing metal turbine arrest impact (120 Hz)
                vrState.rightHapticPulse = 0.95f;
            }

            // Atmospheric Hull Rupture Emergency Foam Canister Deployment (ADR 0088)
            static DWORD s_lastFoamCanisterDeployTime = 0;
            bool bothAtBreach = (leftLocal.z > -0.44f && leftLocal.z < -0.16f && rightLocal.z > -0.44f && rightLocal.z < -0.16f) &&
                                (leftLocal.y > -0.30f && leftLocal.y < 0.20f && rightLocal.y > -0.30f && rightLocal.y < 0.20f);
            bool bothTriggersFoam = (vrState.leftController.trigger > 0.60f && vrState.rightController.trigger > 0.60f);
            bool foamSeamSweep = (fabsf(lVel.x) > 0.30f || fabsf(rVel.x) > 0.30f || fabsf(lVel.y) > 0.30f || fabsf(rVel.y) > 0.30f);
            if (bothAtBreach && bothTriggersFoam && foamSeamSweep && (now - s_lastFoamCanisterDeployTime > 2000)) {
                s_lastFoamCanisterDeployTime = now;
                VR_LOG_INFO("Environmental Gesture: HULL RUPTURE EMERGENCY FOAM CANISTER EXTRUDED (Vx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                vrState.leftHapticPulse = 0.60f; // Acoustic foam jet extrusion (120 Hz)
                vrState.rightHapticPulse = 0.75f; // Rapid cure polymerization pop (200 Hz)
            }

            // Subterranean Hydroponics Plant Specimen Harvesting (ADR 0093)
            static DWORD s_lastHarvestTime = 0;
            bool handAtHydroponicTray = ((rightLocal.z > -0.42f && rightLocal.z < -0.14f && rightLocal.y > -0.55f && rightLocal.y < -0.15f && vrState.rightController.grip > 0.45f && vrState.rightController.grip < 0.80f) ||
                                         (leftLocal.z > -0.42f && leftLocal.z < -0.14f && leftLocal.y > -0.55f && leftLocal.y < -0.15f && vrState.leftController.grip > 0.45f && vrState.leftController.grip < 0.80f));
            bool upwardPluck = (rVel.y > 0.40f || lVel.y > 0.40f);
            if (handAtHydroponicTray && upwardPluck && (now - s_lastHarvestTime > 850)) {
                s_lastHarvestTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONIC SPECIMEN HARVESTED (Vy: %.2f)", (rVel.y > lVel.y) ? rVel.y : lVel.y);
                SendKeyTap('E');
                if (rVel.y > 0.40f) vrState.rightHapticPulse = 0.45f; // Botanical snap click (160 Hz)
                if (lVel.y > 0.40f) vrState.leftHapticPulse = 0.45f;
            }

            // Dark Athena Drone Charging Bay Station Disconnect Jerk (ADR 0094)
            static DWORD s_lastDroneBayJerkTime = 0;
            bool handInDroneAlcove = ((rightLocal.z > -0.44f && rightLocal.z < -0.16f && rightLocal.y > 0.05f && rightLocal.y < 0.38f && vrState.rightController.grip > 0.60f) ||
                                      (leftLocal.z > -0.44f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.38f && vrState.leftController.grip > 0.60f));
            bool rearwardCableYank = (rVel.z > 0.85f || lVel.z > 0.85f);
            if (handInDroneAlcove && rearwardCableYank && (now - s_lastDroneBayJerkTime > 1500)) {
                s_lastDroneBayJerkTime = now;
                VR_LOG_INFO("Environmental Gesture: DRONE RECHARGE UMBILICAL SEVERED (Vz: %.2f)", (rVel.z > lVel.z) ? rVel.z : lVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                if (rVel.z > 0.85f) vrState.rightHapticPulse = 0.90f; // Inductive arc shudder (240 Hz)
                if (lVel.z > 0.85f) vrState.leftHapticPulse = 0.90f;
            }

            // Cybernetic Drone Power Shunt Manual Lever Yank (ADR 0121)
            static DWORD s_lastDroneShuntTime = 0;
            bool handAtDroneShunt = ((rightLocal.z > -0.42f && rightLocal.z < -0.14f && rightLocal.y > -0.15f && rightLocal.y < 0.20f && vrState.rightController.grip > 0.50f) ||
                                     (leftLocal.z > -0.42f && leftLocal.z < -0.14f && leftLocal.y > -0.15f && leftLocal.y < 0.20f && vrState.leftController.grip > 0.50f));
            bool shuntYankDown = (rVel.y < -0.60f || lVel.y < -0.60f);
            if (handAtDroneShunt && shuntYankDown && (now - s_lastDroneShuntTime > 2000)) {
                s_lastDroneShuntTime = now;
                VR_LOG_INFO("Environmental Gesture: DRONE POWER SHUNT BREAKER YANKED DOWNWARD (Vy: %.2f)", (rVel.y < lVel.y) ? rVel.y : lVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                if (rVel.y < -0.60f) vrState.rightHapticPulse = 0.90f; // Inductive shunt discharge shudder (280 Hz)
                if (lVel.y < -0.60f) vrState.leftHapticPulse = 0.90f;
            }

            // Subterranean Cryo-Stasis Chamber Biometric Bypass Override (ADR 0097)
            static DWORD s_lastCryoOverrideTime = 0;
            bool leftAtUpperTerminal = (leftLocal.y > 0.08f && leftLocal.y < 0.32f) &&
                                       (leftLocal.z > -0.38f && leftLocal.z < -0.14f) &&
                                       (vrState.leftController.grip > 0.50f);
            bool rightAtLowerBreaker = (rightLocal.y > -0.28f && rightLocal.y < 0.04f) &&
                                       (rightLocal.z > -0.38f && rightLocal.z < -0.14f) &&
                                       (vrState.rightController.grip > 0.50f);
            bool breakerYankDown = (rVel.y < -0.50f) && (lSpeed < 0.25f);
            if (leftAtUpperTerminal && rightAtLowerBreaker && breakerYankDown && (now - s_lastCryoOverrideTime > 2000)) {
                s_lastCryoOverrideTime = now;
                VR_LOG_INFO("Environmental Gesture: CRYO-STASIS BIOMETRIC BYPASS OVERRIDE (Ry: %.2f, Ly: %.2f)", rVel.y, leftLocal.y);
                SendKeyTap('E');
                SendKeyTap(VK_RETURN);
                vrState.leftHapticPulse = 0.50f; // Electronic card read chirp (220 Hz)
                vrState.rightHapticPulse = 0.80f; // Heavy magnetic breaker disconnect thud (120 Hz)
            }

            // Dark Athena Fusion Reactor Core Rod Manual Insertion (ADR 0100 - Century Milestone!)
            static DWORD s_lastReactorRodInsertTime = 0;
            bool bothGrippingRod = (twoHandDist < 0.32f) && (vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
            bool rodElevated = (leftLocal.y > 0.15f && rightLocal.y > 0.15f);
            bool rodPlunge = (rVel.z < -1.15f && lVel.z < -1.15f && rVel.y < -0.80f && lVel.y < -0.80f);
            if (bothGrippingRod && rodElevated && rodPlunge && (now - s_lastReactorRodInsertTime > 3000)) {
                s_lastReactorRodInsertTime = now;
                VR_LOG_INFO("Environmental Gesture: FUSION REACTOR CORE ROD MANUAL INSERTION (Lz: %.2f, Rz: %.2f, Ly: %.2f, Ry: %.2f)", lVel.z, rVel.z, lVel.y, rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_RETURN);
                vrState.leftHapticPulse = 0.98f; // Catastrophic magnetic confinement shockwave (45 Hz)
                vrState.rightHapticPulse = 0.98f;
            }

            // Subterranean Hydroponics Nutrient Dispenser Pressure Plunger (ADR 0105)
            static DWORD s_lastPlungerStrokeTime = 0;
            static int s_plungerStrokeCount = 0;
            bool handAtPlungerPump = ((rightLocal.z > -0.38f && rightLocal.z < -0.14f && rightLocal.y > -0.15f && rightLocal.y < 0.25f && vrState.rightController.grip > 0.50f) ||
                                      (leftLocal.z > -0.38f && leftLocal.z < -0.14f && leftLocal.y > -0.15f && leftLocal.y < 0.25f && vrState.leftController.grip > 0.50f));
            bool plungerPlunge = (rVel.y < -0.45f || lVel.y < -0.45f);
            if (handAtPlungerPump && plungerPlunge && (now - s_lastPlungerStrokeTime > 220)) {
                s_lastPlungerStrokeTime = now;
                s_plungerStrokeCount++;
                VR_LOG_INFO("Environmental Gesture: HYDROPONIC NUTRIENT PLUNGER STROKE #%d (Vy: %.2f)", s_plungerStrokeCount, (rVel.y < lVel.y) ? rVel.y : lVel.y);
                if (rVel.y < -0.45f) vrState.rightHapticPulse = 0.60f; // Hydraulic displacement slosh (110 Hz)
                if (lVel.y < -0.45f) vrState.leftHapticPulse = 0.60f;
                if (s_plungerStrokeCount >= 3) {
                    s_plungerStrokeCount = 0;
                    VR_LOG_INFO("Environmental Gesture: NUTRIENT ACCUMULATOR RELIEF VALVE CLEARED / IRRIGATION RESTORED");
                    SendKeyTap('E');
                    SendKeyTap('Q');
                    vrState.leftHapticPulse = 0.75f; // Relief valve clack (220 Hz)
                    vrState.rightHapticPulse = 0.75f;
                }
            }

            // Subterranean Hydroponics Nutrient Dosing Solenoid Lever (ADR 0116)
            static DWORD s_lastSolenoidLeverTime = 0;
            bool handAtSolenoidLever = ((rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > -0.25f && rightLocal.y < 0.15f && vrState.rightController.grip > 0.50f) ||
                                        (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > -0.25f && leftLocal.y < 0.15f && vrState.leftController.grip > 0.50f));
            bool solenoidLeverUp = (rVel.y > 0.65f || lVel.y > 0.65f);
            if (handAtSolenoidLever && solenoidLeverUp && (now - s_lastSolenoidLeverTime > 1500)) {
                s_lastSolenoidLeverTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONIC NUTRIENT SOLENOID LEVER THROWN UPWARD (Vy: %.2f)", (rVel.y > lVel.y) ? rVel.y : lVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                if (rVel.y > 0.65f) vrState.rightHapticPulse = 0.80f; // Armature clack & pipe surge (110 Hz)
                if (lVel.y > 0.65f) vrState.leftHapticPulse = 0.80f;
            }

            // Dark Athena Cargo Bay Magnetic Crane Remote Pendant Snatch (ADR 0106)
            static DWORD s_lastCranePendantTime = 0;
            bool handAtOverheadPendant = ((rightLocal.y > 0.18f && rightLocal.z > -0.32f && rightLocal.z < -0.05f && vrState.rightController.grip > 0.55f) ||
                                          (leftLocal.y > 0.18f && leftLocal.z > -0.32f && leftLocal.z < -0.05f && vrState.leftController.grip > 0.55f));
            bool pendantDownwardHaul = (rVel.y < -0.45f || lVel.y < -0.45f);
            if (handAtOverheadPendant && pendantDownwardHaul && (now - s_lastCranePendantTime > 2000)) {
                s_lastCranePendantTime = now;
                VR_LOG_INFO("Environmental Gesture: CARGO GANTRY CRANE PENDANT SNATCHED & HAULED (Vy: %.2f)", (rVel.y < lVel.y) ? rVel.y : lVel.y);
                SendKeyTap('E');
                SendKeyTap('W'); // Engage gantry remote bridge
                if (rVel.y < -0.45f) vrState.rightHapticPulse = 0.70f; // Steel cable tension & relay click (160 Hz)
                if (lVel.y < -0.45f) vrState.leftHapticPulse = 0.70f;
            }

            // Dark Athena EVA Solar Flare Radiation Shelter Bulkhead Seal (ADR 0109)
            static DWORD s_lastSolarBulkheadTime = 0;
            bool bothOverheadRadiationBulkhead = (leftLocal.y > 0.22f && rightLocal.y > 0.22f) &&
                                                 (leftLocal.z > -0.30f && leftLocal.z < 0.05f && rightLocal.z > -0.30f && rightLocal.z < 0.05f) &&
                                                 (vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
            bool violentDownwardBulkheadSlam = (lVel.y < -1.10f && rVel.y < -1.10f);
            if (bothOverheadRadiationBulkhead && violentDownwardBulkheadSlam && (now - s_lastSolarBulkheadTime > 2500)) {
                s_lastSolarBulkheadTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR FLARE RADIATION SHELTER BULKHEAD SLAMMED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKeyTap('E');
                SendKeyTap('C');
                SendKeyTap(VK_SPACE);
                vrState.leftHapticPulse = 0.98f;  // Deep lead door structural impact shockwave (40 Hz)
                vrState.rightHapticPulse = 0.98f;
            }

            // Dark Athena EVA Solar Collector Debris Shield Ejection (ADR 0117)
            static DWORD s_lastShieldEjectTime = 0;
            bool bothAtTrussMount = (leftLocal.z > -0.40f && leftLocal.z < -0.14f && rightLocal.z > -0.40f && rightLocal.z < -0.14f) &&
                                    (leftLocal.y > 0.10f && leftLocal.y < 0.35f && rightLocal.y > 0.10f && rightLocal.y < 0.35f) &&
                                    (vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
            bool dualRearwardYank = (lVel.z > 0.85f && rVel.z > 0.85f);
            if (bothAtTrussMount && dualRearwardYank && (now - s_lastShieldEjectTime > 2500)) {
                s_lastShieldEjectTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DEBRIS SHIELD JETTISONED (Lz: %.2f, Rz: %.2f)", lVel.z, rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_RETURN);
                vrState.leftHapticPulse = 0.95f;  // Pyrotechnic explosive pin severance shockwave (240 Hz)
                vrState.rightHapticPulse = 0.95f;
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

        // Tactical Radio Transceiver Intercept / Eavesdropping (ADR 0071)
        static DWORD s_radioHoldStartTime = 0;
        static DWORD s_lastRadioInterceptTime = 0;
        bool rightAtEar = (rightLocal.x > 0.16f && rightLocal.y > 0.05f && rightLocal.y < 0.22f && rightLocal.z > -0.15f && rightLocal.z < 0.08f);
        bool leftAtEar = (leftLocal.x < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.22f && leftLocal.z > -0.15f && leftLocal.z < 0.08f);
        bool earGripHeld = (rightAtEar && vrState.rightController.grip > 0.50f) || (leftAtEar && vrState.leftController.grip > 0.50f);

        if (earGripHeld) {
            if (s_radioHoldStartTime == 0) {
                s_radioHoldStartTime = now;
            } else if ((now - s_radioHoldStartTime >= 120) && (now - s_lastRadioInterceptTime > 1200)) {
                s_lastRadioInterceptTime = now;
                VR_LOG_INFO("Tactical Intercept: GUARD RADIO TRANSCEIVER EAVESDROPPING ENGAGED");
                SendKeyTap(VK_TAB); // PDA / Tactical audio channel
                if (rightAtEar) vrState.rightHapticPulse = 0.40f; // Squelch burst (220 Hz)
                if (leftAtEar) vrState.leftHapticPulse = 0.40f;
            }
        } else {
            s_radioHoldStartTime = 0;
        }

        // EVA Spacewalk Solar Radiation Shield Visor Downward Pull (ADR 0086)
        static DWORD s_lastSolarVisorTime = 0;
        bool rightAtBrow = (rightLocal.y > 0.18f && rightLocal.z > -0.18f && rightLocal.z < 0.05f && fabsf(rightLocal.x) < 0.20f && vrState.rightController.grip > 0.50f);
        bool leftAtBrow = (leftLocal.y > 0.18f && leftLocal.z > -0.18f && leftLocal.z < 0.05f && fabsf(leftLocal.x) < 0.20f && vrState.leftController.grip > 0.50f);
        bool visorPullDown = (rightAtBrow && rVel.y < -0.55f) || (leftAtBrow && lVel.y < -0.55f);

        if (visorPullDown && (now - s_lastSolarVisorTime > 1200)) {
            s_lastSolarVisorTime = now;
            VR_LOG_INFO("Orbital EVA Gesture: POLARIZED SOLAR RADIATION VISOR PULLED DOWN");
            SendKeyTap('Q'); // Toggle optical tint filter
            if (rightAtBrow) vrState.rightHapticPulse = 0.65f; // Solid mechanical visor latch click (150 Hz)
            if (leftAtBrow) vrState.leftHapticPulse = 0.65f;
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

    // Heavy Mech Rotary Chaingun Dual-Hand Recoil Compensation (ADR 0107)
    static DWORD s_lastRecoilCompTime = 0;
    bool isDualHandBracedChaingun = s_twoHandedStabilizationActive && (vrState.rightController.trigger > 0.80f) && (vrState.leftController.grip > 0.60f);
    if (isDualHandBracedChaingun && (s_triggerHoldStartTime != 0) && (now - s_triggerHoldStartTime > 300)) {
        if (now - s_lastRecoilCompTime > 90) {
            s_lastRecoilCompTime = now;
            vrState.rightHapticPulse = 0.65f; // Mechanical bolt cycling (160 Hz)
            vrState.leftHapticPulse = 0.70f;  // Heavy hydraulic chassis counter-torque (110 Hz)
            s_recoilKickPitch *= 0.35f;       // Active kinematic muzzle climb clamp
        }
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

    // Interactive Inmate Gambling Card Flipping & Dealing (ADR 0075)
    static DWORD s_lastCardDealTime = 0;
    bool cardHandProximity = (rightLocal.y > -0.45f && rightLocal.y < -0.10f) &&
                             (rightLocal.z > -0.40f && rightLocal.z < -0.15f);
    bool cardPinchGrip = (vrState.rightController.grip > 0.40f && vrState.rightController.grip < 0.75f) &&
                         (vrState.rightController.trigger < 0.30f);
    bool cardFlick = (fabsf(rVel.x) > 0.60f || rVel.z < -0.65f);
    if (cardHandProximity && cardPinchGrip && cardFlick && !s_physicalBlockEngaged && (now - s_lastCardDealTime > 300)) {
        s_lastCardDealTime = now;
        VR_LOG_INFO("Diegetic Interaction: INMATE GAMBLING CARD FLIPPED / DEALT (Vx: %.2f, Vz: %.2f)", rVel.x, rVel.z);
        SendKeyTap('E');
        SendKeyTap(VK_RETURN);
        vrState.rightHapticPulse = 0.35f; // Crisp paper card friction micro-haptic (240 Hz)
    }

    // Inmate Pit Brawl Dirt / Ash Throw Blind (ADR 0065)
    static DWORD s_lastAshThrowTime = 0;
    bool ashThrowFlick = (s_isPhysicallyCrouched || s_isDeepProne) &&
                         (vrState.rightController.grip < 0.40f && vrState.rightController.trigger < 0.40f) &&
                         (rVel.y > 0.80f && rVel.z < -0.80f);
    if (ashThrowFlick && (now - s_lastAshThrowTime > 1200)) {
        s_lastAshThrowTime = now;
        VR_LOG_INFO("Combat Dirty Tactic: DIRT / ASH THROWN INTO ENEMY EYES (Vy: %.2f, Vz: %.2f)", rVel.y, rVel.z);
        SendKeyTap('E');
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        vrState.rightHapticPulse = 0.50f; // Fine particulate scatter burst (180 Hz)
    }

    // Inmate Pit Brawl Sand / Ash Throw Two-Handed Fan Scatter (ADR 0085)
    static DWORD s_lastFanScatterTime = 0;
    bool bothHandsLowSand = (s_isPhysicallyCrouched || s_isDeepProne) &&
                            (leftLocal.y < -0.40f && rightLocal.y < -0.40f) &&
                            (vrState.leftController.grip < 0.45f && vrState.rightController.grip < 0.45f);
    bool fanScatterOutward = (lVel.x < -0.80f && rVel.x > 0.80f) && (lVel.y > 0.40f || rVel.y > 0.40f);
    if (bothHandsLowSand && fanScatterOutward && (now - s_lastFanScatterTime > 1400)) {
        s_lastFanScatterTime = now;
        VR_LOG_INFO("Combat Dirty Tactic: DUAL-HANDED SAND / ASH FAN SCATTER (Lx: %.2f, Rx: %.2f)", lVel.x, rVel.x);
        SendKeyTap('E');
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        vrState.leftHapticPulse = 0.65f; // Dual wide-arc particulate scatter pulses (180 Hz)
        vrState.rightHapticPulse = 0.65f;
    }

    // Inmate Pit Brawl Dirt / Ash Two-Handed Fan Clench & Burst (ADR 0104)
    static DWORD s_lastFanClenchBurstTime = 0;
    static DWORD s_fanClenchStartTime = 0;
    static bool s_handsWereClenchedChest = false;

    float dxHandsFan = leftLocal.x - rightLocal.x;
    float dyHandsFan = leftLocal.y - rightLocal.y;
    float dzHandsFan = leftLocal.z - rightLocal.z;
    float distHandsFan = sqrtf(dxHandsFan * dxHandsFan + dyHandsFan * dyHandsFan + dzHandsFan * dzHandsFan);

    bool handsAtChest = (leftLocal.y > -0.25f && leftLocal.y < 0.05f) &&
                        (rightLocal.y > -0.25f && rightLocal.y < 0.05f) &&
                        (distHandsFan < 0.18f);
    bool bothFanGripsClenched = (vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);

    if (handsAtChest && bothFanGripsClenched && !s_physicalBlockEngaged) {
        if (!s_handsWereClenchedChest) {
            s_handsWereClenchedChest = true;
            s_fanClenchStartTime = now;
            vrState.leftHapticPulse = 0.45f;  // Friction grind during clench (200 Hz)
            vrState.rightHapticPulse = 0.45f;
        }
    } else if (s_handsWereClenchedChest) {
        // Evaluate burst if fists were clenched together recently
        if (now - s_fanClenchStartTime < 800) {
            bool explosiveFanOutward = (lVel.x < -1.10f && rVel.x > 1.10f);
            if (explosiveFanOutward && (now - s_lastFanClenchBurstTime > 2000)) {
                s_lastFanClenchBurstTime = now;
                VR_LOG_INFO("Combat Dirty Tactic: DIRT/ASH TWO-HANDED FAN CLENCH & BURST (Lx: %.2f, Rx: %.2f)", lVel.x, rVel.x);
                SendKeyTap('E');
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKeyTap(VK_TAB);
                vrState.leftHapticPulse = 0.70f;  // Wide particulate cloud rush (175 Hz)
                vrState.rightHapticPulse = 0.70f;
            }
        }
        if (!bothFanGripsClenched || distHandsFan > 0.35f) {
            s_handsWereClenchedChest = false;
        }
    }

    // Inmate Pit Brawl Dirt / Ash Eye Recovery Blink Gesture (ADR 0073)
    static DWORD s_lastEyeRecoveryTime = 0;
    bool rightWipingEyes = (rightLocal.z > -0.22f && rightLocal.z < -0.05f) &&
                           (rightLocal.y > -0.05f && rightLocal.y < 0.10f) &&
                           (fabsf(rVel.x) > 0.50f);
    bool leftWipingEyes = (leftLocal.z > -0.22f && leftLocal.z < -0.05f) &&
                          (leftLocal.y > -0.05f && leftLocal.y < 0.10f) &&
                          (fabsf(lVel.x) > 0.50f);
    bool rapidHeadShake = (fabsf(s_lastHmdDeltaYaw) > 0.15f);

    if ((rightWipingEyes || leftWipingEyes || rapidHeadShake) && (now - s_lastEyeRecoveryTime > 1200)) {
        s_lastEyeRecoveryTime = now;
        VR_LOG_INFO("Combat Defense: CORNEAL BLINK / DIRT WIPED FROM EYES (%s)",
            rapidHeadShake ? "Rapid Head Shake" : "Forearm Eye Wipe");
        SendKeyTap(VK_SPACE); // Visual overlay recovery accelerator
        vrState.rightHapticPulse = 0.30f; // Fine corneal blink micro-haptic (180 Hz)
        vrState.leftHapticPulse = 0.30f;
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

    // Inmate Bunk Shiv Sheath Concealment Under Mattress Frame (ADR 0110)
    static DWORD s_lastBunkConcealTime = 0;
    bool leftLiftingMattress = (s_isPhysicallyCrouched || s_isDeepProne) &&
                               s_leftGripDown && (leftLocal.y > -0.45f && leftLocal.y < -0.15f);
    bool rightThrustingIntoSlat = (rightLocal.z > -0.42f && rightLocal.z < -0.14f) && (rVel.z < -0.65f);
    if (leftLiftingMattress && rightThrustingIntoSlat && (now - s_lastBunkConcealTime > 1500)) {
        s_lastBunkConcealTime = now;
        VR_LOG_INFO("Environmental Gesture: INMATE BUNK SHIV SHEATH CONCEALED UNDER MATTRESS FRAME (Vz: %.2f m/s)", rVel.z);
        SendKeyTap('3'); // Holster / stow shiv weapon
        SendKeyTap('E');
        vrState.leftHapticPulse = 0.35f;  // Mattress wire tension hum (120 Hz)
        vrState.rightHapticPulse = 0.60f; // Spring-steel sheath latch click (240 Hz)
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
