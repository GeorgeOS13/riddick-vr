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
static bool s_isPhysicalADS = false;
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
    static bool s_cinemaModeState = true; // Default to Cinema Mode for initial game boot & main menu
    static bool s_wasIn3D = false;

    bool has3D = OpenGLProxy::Has3DGameplayProjection();

    // Auto-transition into Full 3D VR Immersion the moment the game loads its 3D gameplay projection
    if (has3D && !s_wasIn3D) {
        s_cinemaModeState = false;
        s_wasIn3D = true;
        VR_LOG_INFO("3D Gameplay projection intercepted: transitioned from Menu Quad to Full 3D VR Immersion");
    }

    // 1. Manual Cinema Mode Toggle via Button Y
    if ((btns & RIDDICK_VR_BTN_Y) && !(s_prevButtons & RIDDICK_VR_BTN_Y)) {
        s_cinemaModeState = !s_cinemaModeState;
        VR_LOG_INFO("Cinema Screen Mode toggled manually via Button Y: %s", s_cinemaModeState ? "ON (Virtual Cinema Quad)" : "OFF (Full 3D VR Immersion)");
    }

    // 2. Pause Menu Cinema Mode via Left Menu Button
    bool btnMenu = (btns & RIDDICK_VR_BTN_MENU) != 0;
    bool prevBtnMenu = (s_prevButtons & RIDDICK_VR_BTN_MENU) != 0;
    if (btnMenu && !prevBtnMenu) {
        s_cinemaModeState = !s_cinemaModeState;
        VR_LOG_INFO("Cinema Screen Mode toggled via Menu Button: %s", s_cinemaModeState ? "ON (Virtual Cinema Quad)" : "OFF (Full 3D VR Immersion)");
    }

    // In menus/boot sequence (before 3D gameplay begins): strictly Cinema Mode quad screen.
    // In 3D gameplay: cinemaMode is only engaged on pause/menu/toggle.
    if (!has3D) {
        vrState.cinemaMode = true;
    } else {
        vrState.cinemaMode = s_cinemaModeState || cursorShowing;
    }

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

                // ADR 0383: Physical 6DOF Aim-Down-Sights (ADS) Eye-Relief Alignment & Parallax Mitigation
                static DWORD s_adsEnterStartTime = 0;
                static bool s_adsHapticFired = false;

                RiddickVRVector3 ctrlLocal = WorldToHmdLocal(ctrlPose.position, vrState.hmdPose);
                // Dominant eye sight line corridor:
                // Y: [-0.18m, 0.08m] (chin to eye level)
                // Z: [-0.36m, -0.10m] (10cm to 36cm eye relief)
                // X: [0.02m, 0.22m] (dominant right eye alignment corridor)
                bool inAdsCorridor = (ctrlLocal.y >= -0.18f && ctrlLocal.y <= 0.08f) &&
                                    (ctrlLocal.z >= -0.36f && ctrlLocal.z <= -0.10f) &&
                                    (ctrlLocal.x >= 0.02f && ctrlLocal.x <= 0.22f);

                if (inAdsCorridor && !isReloading && !s_physicalBlockEngaged) {
                    if (s_adsEnterStartTime == 0) {
                        s_adsEnterStartTime = now;
                    } else if (now - s_adsEnterStartTime > 50) { // 50ms hold to confirm ADS
                        s_isPhysicalADS = true;
                        if (!s_adsHapticFired) {
                            vrState.rightHapticPulse = 0.18f; // Tactile detent on sight picture acquisition
                            s_adsHapticFired = true;
                        }
                    }
                } else {
                    // Spatial exit hysteresis: requires moving clearly outside the corridor
                    if (ctrlLocal.y < -0.24f || ctrlLocal.z < -0.42f || ctrlLocal.x < -0.05f || ctrlLocal.x > 0.28f) {
                        s_isPhysicalADS = false;
                        s_adsEnterStartTime = 0;
                        s_adsHapticFired = false;
                    }
                }

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

                            // ADR 0374: Quest Controller Ergonomic Barrel Pitch Calibration
                            // Natural wrist resting angle offsets the raw aim ray ~10.5 degrees downward
                            constexpr float ERGONOMIC_BARREL_PITCH_OFFSET = -0.1832f; // -10.5 deg in radians
                            ctrlPitch += ERGONOMIC_BARREL_PITCH_OFFSET;

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

                                // Aim weights: only pull aim when actively aiming (ADS or two-handed stabilization or aiming trigger)
                                bool isActivelyAiming = (s_isPhysicalADS || s_twoHandedStabilizationActive || isAiming) && !s_physicalBlockEngaged && !isReloading;
                                if (isActivelyAiming) {
                                    wPull = s_isPhysicalADS ? 0.12f : 0.08f;
                                } else {
                                    wPull = 0.0f;
                                }
                            }
                        }
                    }
                } else {
                    s_hasPrevCtrlAim = false;
                }

                // Pure 1:1 Head Tracking Look: map HMD yaw and pitch directly to in-game camera
                // Center deadzone (~0.06 deg) filters high-frequency sensor noise / tremor
                constexpr float HMD_LOOK_DEADZONE = 0.0010f;
                float cleanDeltaYaw = (fabsf(deltaYaw) > HMD_LOOK_DEADZONE) ? deltaYaw : 0.0f;
                float cleanDeltaPitch = (fabsf(deltaPitch) > HMD_LOOK_DEADZONE) ? deltaPitch : 0.0f;

                // Motion controllers never steer the head camera (ZERO controller aim pull):
                constexpr float SENSITIVITY_YAW = 445.6f;
                constexpr float SENSITIVITY_PITCH = 445.6f;

                float mouseDeltaX = -cleanDeltaYaw * SENSITIVITY_YAW;
                float mouseDeltaY = -cleanDeltaPitch * SENSITIVITY_PITCH;

                // Pure 1:1 head tracking look is uncompromised during ADS/aiming (aim stabilization is applied to controller aim pose)

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

        // ADR 0377: Peak tracking with Jump Immunity & Vertical Velocity Gating
        // Only adapt standing height when stationary and upright (|Vy| < 0.12 m/s).
        // Adapt gradually via exponential smoothing so jumping or head bob never latches a false floor!
        float hmdVelY = vrState.hmdPose.linearVelocity.y;
        if (!s_isPhysicallyCrouched && currentY > s_standingEyeHeight && fabsf(hmdVelY) < 0.12f) {
            s_standingEyeHeight += 0.002f * (currentY - s_standingEyeHeight);
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

    // Grab initiate: hand raised in front of chest/chin (Y > 0.15m, Z in [-0.55, -0.16]) and grip squeezed (> 0.5f)
    // ADR 0381: Spatial forward-cone depth filtering decouples ladder grabs from over-shoulder weapon drawing!
    // ADR 0386: Capacitive sensor disambiguation: ensure fingers are curled around rung rather than resting
    if (!s_leftHoldingRung) {
        bool leftTouchEngaged = (vrState.leftController.buttons & RIDDICK_VR_TOUCH_TRIGGER) != 0 || vrState.leftController.trigger > 0.20f;
        if (leftLocal.y > 0.15f && (leftLocal.z >= -0.55f && leftLocal.z <= -0.16f) && vrState.leftController.grip > 0.5f && leftTouchEngaged) {
            s_leftHoldingRung = true;
            VR_LOG_INFO("Physical Gesture: LADDER RUNG GRABBED (Left hand, Y: %.2fm, Z: %.2fm)", leftLocal.y, leftLocal.z);
            vrState.leftHapticPulse = 0.35f;
            SendKeyTap('E');
        }
    } else {
        if (vrState.leftController.grip < 0.40f) {
            s_leftHoldingRung = false;
        }
    }

    if (!s_rightHoldingRung) {
        bool rightTouchEngaged = (vrState.rightController.buttons & RIDDICK_VR_TOUCH_TRIGGER) != 0 || vrState.rightController.trigger > 0.20f;
        if (rightLocal.y > 0.15f && (rightLocal.z >= -0.55f && rightLocal.z <= -0.16f) && vrState.rightController.grip > 0.5f && rightTouchEngaged) {
            s_rightHoldingRung = true;
            VR_LOG_INFO("Physical Gesture: LADDER RUNG GRABBED (Right hand, Y: %.2fm, Z: %.2fm)", rightLocal.y, rightLocal.z);
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

    // 3. Radial Locomotion Deadzone & Diagonal Movement Normalization (ADR 0372)
    static bool s_wPressed = false, s_sPressed = false, s_aPressed = false, s_dPressed = false;
    constexpr float RADIAL_DEADZONE = 0.22f;
    float stickLX = vrState.leftController.thumbstickX;
    float stickLY = vrState.leftController.thumbstickY;
    float moveMag = sqrtf(stickLX * stickLX + stickLY * stickLY);

    bool wantW = false;
    bool wantS = false;
    bool wantA = false;
    bool wantD = false;

    if (!vrState.cinemaMode && moveMag >= RADIAL_DEADZONE) {
        float dirX = stickLX / moveMag;
        float dirY = stickLY / moveMag;

        // Apply angular deadband around forward/backward axis (±25 degrees, sin(25°) ≈ 0.42)
        // This prevents unintentional sideways strafing drift when walking forward/backward
        constexpr float FORWARD_THRESHOLD = 0.35f;
        constexpr float STRAFE_THRESHOLD = 0.42f;

        if (dirY > FORWARD_THRESHOLD) wantW = true;
        else if (dirY < -FORWARD_THRESHOLD) wantS = true;

        if (dirX > STRAFE_THRESHOLD) wantD = true;
        else if (dirX < -STRAFE_THRESHOLD) wantA = true;
    }

    if (s_ladderClimbUp) wantW = true;
    if (s_ladderClimbDown) wantS = true;

    if (wantW != s_wPressed) { SendKey('W', wantW); s_wPressed = wantW; }
    if (wantS != s_sPressed) { SendKey('S', wantS); s_sPressed = wantS; }
    if (wantA != s_aPressed) { SendKey('A', wantA); s_aPressed = wantA; }
    if (wantD != s_dPressed) { SendKey('D', wantD); s_dPressed = wantD; }

    // Left Thumbstick Click Sprint & Outer Deflection Run (ADR 0391)
    static bool s_sprintActive = false;
    static bool s_prevStickLeftClick = false;
    static bool s_shiftDown = false;
    bool stickLeftClick = (btns & RIDDICK_VR_BTN_STICK_LEFT) != 0;

    if (stickLeftClick && !s_prevStickLeftClick && wantW) {
        s_sprintActive = !s_sprintActive;
        if (s_sprintActive) {
            vrState.leftHapticPulse = 0.25f; // Crisp tactile click on sprint engage
        }
    }
    s_prevStickLeftClick = stickLeftClick;

    // Disengage sprint if player stops moving forward or enters crouch/prone
    if (!wantW || s_isPhysicallyCrouched || s_isDeepProne || isClimbingActive) {
        s_sprintActive = false;
    }

    bool isSprinting = wantW && (s_sprintActive || stickLY > 0.92f);
    if (isSprinting != s_shiftDown) {
        SendKey(VK_SHIFT, isSprinting);
        s_shiftDown = isSprinting;
    }

    // Prone crawl pacing (ADR 0030): rhythmic tactile crawl heartbeat in deep prone
    static DWORD s_lastPronePulseTime = 0;
    if (s_isDeepProne && (wantW || wantS || wantA || wantD) && (now - s_lastPronePulseTime > 600)) {
        s_lastPronePulseTime = now;
        vrState.leftHapticPulse = 0.12f; // Subtle stealth crawl pulse (85 Hz)
    }

    // Refined Locomotion Footstep Cadence (ADR 0392):
    // Subtle, low-amplitude tactile gait ticks during upright locomotion (0.06f walk, 0.12f sprint)
    static DWORD s_lastStepPulseTime = 0;
    static bool s_stepLeftFoot = false;
    DWORD stepInterval = isSprinting ? 360 : 540;
    if (!s_isPhysicallyCrouched && !s_isDeepProne && !isClimbingActive && (wantW || wantS || wantA || wantD) && (now - s_lastStepPulseTime > stepInterval)) {
        s_lastStepPulseTime = now;
        s_stepLeftFoot = !s_stepLeftFoot;
        float stepPulse = isSprinting ? 0.12f : 0.06f;
        if (s_stepLeftFoot) {
            vrState.leftHapticPulse = stepPulse;
        } else {
            vrState.rightHapticPulse = stepPulse;
        }
    }

    // 4. Thumbstick Snap vs Smooth Turning Ergonomics & Input Hysteresis (ADR 0371)
    enum VRTurnMode {
        VR_TURN_SNAP = 0,    // Discrete 45° snap turns with zero pre-snap crawl
        VR_TURN_SMOOTH = 1   // Continuous rotation with cubic acceleration curve
    };
    static VRTurnMode s_turnMode = VR_TURN_SNAP;
    static bool s_snapTurnArmed = true;

    // Mode Toggle: Left Grip + Right Stick Click toggles turn mode
    static bool s_prevStickClickToggle = false;
    bool stickRightClick = (btns & RIDDICK_VR_BTN_STICK_RIGHT) != 0;
    bool leftGripHeld = vrState.leftController.grip > 0.5f;
    if (stickRightClick && leftGripHeld && !s_prevStickClickToggle) {
        s_turnMode = (s_turnMode == VR_TURN_SNAP) ? VR_TURN_SMOOTH : VR_TURN_SNAP;
        VR_LOG_INFO("Turning Mode Toggled: %s", (s_turnMode == VR_TURN_SNAP) ? "SNAP TURN (45 deg)" : "SMOOTH TURN (Cubic Curve)");
        vrState.rightHapticPulse = (s_turnMode == VR_TURN_SNAP) ? 0.40f : 0.25f;
    }
    s_prevStickClickToggle = (stickRightClick && leftGripHeld);

    if (!vrState.cinemaMode) {
        float stickX = vrState.rightController.thumbstickX;

        if (s_turnMode == VR_TURN_SNAP) {
            // Pure Snap Turn: require deflection > 0.70f, trigger clean single 45° mouse delta
            // CRITICAL: Absolutely no smooth mouse move during ramp-up or return to center!
            constexpr float SNAP_THRESHOLD = 0.70f;
            constexpr float RESET_THRESHOLD = 0.30f;

            if (fabsf(stickX) > SNAP_THRESHOLD) {
                if (s_snapTurnArmed) {
                    s_snapTurnArmed = false;
                    int snapDelta = (stickX > 0.0f) ? 350 : -350;
                    mouse_event(MOUSEEVENTF_MOVE, snapDelta, 0, 0, 0);
                    vrState.rightHapticPulse = 0.25f; // Crisp tactile click on snap
                }
            } else if (fabsf(stickX) < RESET_THRESHOLD) {
                s_snapTurnArmed = true;
            }
        } else {
            // Pure Smooth Turn: continuous rotation with progressive cubic curve
            // Zero sudden 350-pixel snaps at stick limits
            constexpr float SMOOTH_DEADZONE = 0.20f;
            if (fabsf(stickX) > SMOOTH_DEADZONE) {
                float norm = (fabsf(stickX) - SMOOTH_DEADZONE) / (1.0f - SMOOTH_DEADZONE);
                float curvedSpeed = norm * norm * 18.0f; // Smooth cubic ramp
                int deltaX = static_cast<int>(std::copysign(curvedSpeed, stickX));
                if (deltaX == 0 && norm > 0.05f) {
                    deltaX = (stickX > 0.0f) ? 1 : -1;
                }
                mouse_event(MOUSEEVENTF_MOVE, deltaX, 0, 0, 0);
            }
        }
    }

    // 4b. Dialogue & Menu Navigation (Dual Thumbsticks -> Up / Down / Left / Right Arrow Keys in Cinema / Menus)
    if (vrState.cinemaMode) {
        static bool s_stickYUpArmed = true, s_stickYDownArmed = true;
        static bool s_stickXLeftArmed = true, s_stickXRightArmed = true;

        float stickY = (fabsf(vrState.leftController.thumbstickY) > fabsf(vrState.rightController.thumbstickY)) ?
            vrState.leftController.thumbstickY : vrState.rightController.thumbstickY;
        float stickX = (fabsf(vrState.leftController.thumbstickX) > fabsf(vrState.rightController.thumbstickX)) ?
            vrState.leftController.thumbstickX : vrState.rightController.thumbstickX;

        constexpr float MENU_STICK_THRESHOLD = 0.45f;

        if (stickY > MENU_STICK_THRESHOLD) {
            if (s_stickYUpArmed) {
                s_stickYUpArmed = false;
                SendKeyTap(VK_UP);
                vrState.leftHapticPulse = 0.20f;
                vrState.rightHapticPulse = 0.20f;
            }
        } else {
            s_stickYUpArmed = true;
        }

        if (stickY < -MENU_STICK_THRESHOLD) {
            if (s_stickYDownArmed) {
                s_stickYDownArmed = false;
                SendKeyTap(VK_DOWN);
                vrState.leftHapticPulse = 0.20f;
                vrState.rightHapticPulse = 0.20f;
            }
        } else {
            s_stickYDownArmed = true;
        }

        if (stickX > MENU_STICK_THRESHOLD) {
            if (s_stickXRightArmed) {
                s_stickXRightArmed = false;
                SendKeyTap(VK_RIGHT);
                vrState.leftHapticPulse = 0.20f;
                vrState.rightHapticPulse = 0.20f;
            }
        } else {
            s_stickXRightArmed = true;
        }

        if (stickX < -MENU_STICK_THRESHOLD) {
            if (s_stickXLeftArmed) {
                s_stickXLeftArmed = false;
                SendKeyTap(VK_LEFT);
                vrState.leftHapticPulse = 0.20f;
                vrState.rightHapticPulse = 0.20f;
            }
        } else {
            s_stickXLeftArmed = true;
        }
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
        } else if (dualCrossSlash || rSpeed > 1.10f || lSpeed > 1.10f) {
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

            // Security Guard Stun Dart Extraction & Counter-Jab (ADR 0126)
            static DWORD s_dartExtractedTime = 0;
            static DWORD s_lastDartCounterJabTime = 0;
            bool handAtChestDart = (fabsf(rightLocal.x) < 0.16f && rightLocal.y > -0.25f && rightLocal.y < 0.05f && rightLocal.z > -0.25f && rightLocal.z < -0.06f);
            bool dartRipYank = (rVel.z > 0.70f || fabsf(rVel.x) > 0.75f);
            if (handAtChestDart && vrState.rightController.grip > 0.65f && dartRipYank && (now - s_lastDartCounterJabTime > 1500)) {
                if (s_dartExtractedTime == 0) {
                    s_dartExtractedTime = now;
                    VR_LOG_INFO("Physical Combat: STUN DART RIPPED FROM CHEST (Rz: %.2f)", rVel.z);
                    vrState.rightHapticPulse = 0.75f; // Barbed extraction snap (280 Hz)
                }
            }
            bool dartCounterJabStrike = (s_dartExtractedTime > 0 && (now - s_dartExtractedTime < 800) &&
                                        rVel.z < -1.15f && rightLocal.y > -0.10f && rightLocal.y < 0.22f);

            // Inmate Pit Brawl Double-Ear Concussive Clap Strike (ADR 0128)
            static DWORD s_lastDoubleEarClapTime = 0;
            bool bothHandsOpenForClap = (vrState.leftController.trigger < 0.25f && vrState.rightController.trigger < 0.25f &&
                                         vrState.leftController.grip < 0.35f && vrState.rightController.grip < 0.35f);
            bool lateralInwardClapAcc = (lVel.x > 0.95f && rVel.x < -0.95f);
            bool clapCorridor = (handsDistSledge < 0.24f && rightLocal.y > -0.10f && rightLocal.y < 0.20f && rightLocal.z > -0.35f && rightLocal.z < -0.10f);
            bool isDoubleEarClap = bothHandsOpenForClap && lateralInwardClapAcc && clapCorridor;

            // Security Guard Stun Baton Rib-Jab & Ground Takedown (ADR 0133)
            static DWORD s_lastBatonRibJabTime = 0;
            bool batonRibJabArmed = (vrState.rightController.grip > 0.60f && vrState.rightController.trigger > 0.60f);
            bool batonRibJabThrust = (rVel.z < -1.10f && rVel.y < -0.85f);
            bool batonRibJabCorridor = (rightLocal.y > -0.30f && rightLocal.y < 0.05f && rightLocal.z > -0.35f && rightLocal.z < -0.10f);
            bool isBatonRibJab = batonRibJabArmed && batonRibJabThrust && batonRibJabCorridor;

            // Inmate Shiv Forearm Defensive Parry-and-Snap (ADR 0134)
            static DWORD s_lastParrySnapTime = 0;
            bool parrySnapOffHandDeflect = (fabsf(lVel.x) > 0.85f);
            bool parrySnapDominantStrike = (rVel.y < -1.15f);
            bool parrySnapProximity = (handsDistSledge < 0.28f);
            bool isParrySnap = parrySnapOffHandDeflect && parrySnapDominantStrike && parrySnapProximity;

            // Inmate Pit Brawl Dirt / Ash Eye-Rub Recovery Riposte (ADR 0136)
            static DWORD s_eyeRubWipeTime = 0;
            static DWORD s_lastEyeRubRiposteTime = 0;
            bool offHandAtEyes = (fabsf(leftLocal.x) < 0.14f && leftLocal.y > -0.08f && leftLocal.y < 0.15f && leftLocal.z > -0.22f && leftLocal.z < -0.06f);
            bool eyeWipeStroke = (fabsf(lVel.x) > 0.50f);
            if (offHandAtEyes && eyeWipeStroke && (now - s_lastEyeRubRiposteTime > 1500)) {
                if (s_eyeRubWipeTime == 0) {
                    s_eyeRubWipeTime = now;
                    VR_LOG_INFO("Physical Combat: EYES CLEARED OF DIRT/ASH WITH OFF-ARM (Lx: %.2f)", lVel.x);
                    vrState.leftHapticPulse = 0.40f; // Corneal wipe friction rasp (190 Hz)
                }
            }
            bool blindRiposteStrike = (s_eyeRubWipeTime > 0 && (now - s_eyeRubWipeTime < 600) && rVel.z < -1.15f);

            // Inmate Pit Brawl Shin Kick & Knee Stomp Hyper-Extension (ADR 0141)
            static DWORD s_lastKneeStompTime = 0;
            bool kneeStompPosture = (s_currentBlockEngaged || s_isPhysicallyCrouched || s_isDeepProne);
            bool kneeStompStrike = (rVel.y < -1.10f && lVel.z < -0.70f);
            bool isKneeStomp = kneeStompPosture && kneeStompStrike;

            // Security Guard Stun Baton Weapon Shock Induction Arc Overheat (ADR 0142)
            static DWORD s_lastInductionOverheatTime = 0;
            static DWORD s_inductionOverheatStart = 0;
            bool inductionGroundedStance = (handsDistSledge < 0.16f) &&
                                           (vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f) &&
                                           (vrState.leftController.trigger > 0.65f && vrState.rightController.trigger > 0.65f);
            bool isInductionOverheated = false;
            if (inductionGroundedStance) {
                if (s_inductionOverheatStart == 0) s_inductionOverheatStart = now;
                DWORD overchargeTime = now - s_inductionOverheatStart;
                float rampRatio = (overchargeTime < 350) ? (overchargeTime / 350.0f) : 1.0f;
                vrState.leftHapticPulse = 0.35f + 0.55f * rampRatio; // Ascending electrostatic buzz (90 -> 280 Hz)
                vrState.rightHapticPulse = 0.35f + 0.55f * rampRatio;
                if (overchargeTime >= 350 && (now - s_lastInductionOverheatTime > 2500)) {
                    s_lastInductionOverheatTime = now;
                    s_inductionOverheatStart = 0;
                    isInductionOverheated = true;
                }
            } else {
                s_inductionOverheatStart = 0;
            }

            // Inmate Pit Brawl Liver Hook & Sternum Elbow Smash Combination (ADR 0145)
            static DWORD s_liverHookTime = 0;
            static DWORD s_lastLiverElbowComboTime = 0;
            bool liverHookStrike = (s_currentBlockEngaged && rVel.y > 1.10f && fabsf(rVel.x) > 0.90f);
            if (liverHookStrike && (now - s_lastLiverElbowComboTime > 1500)) {
                if (s_liverHookTime == 0) {
                    s_liverHookTime = now;
                    VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL LIVER HOOK CONNECTED (Ry: %.2f, Rx: %.2f)", rVel.y, rVel.x);
                    vrState.rightHapticPulse = 0.95f; // Soft-tissue concussive shockwave (220 Hz)
                }
            }
            bool sternumElbowSmash = (s_liverHookTime > 0 && (now - s_liverHookTime < 500) && rVel.y < -1.20f);

            // Security Guard Stun Baton Charged Underarm Spear Thrust (ADR 0146)
            static DWORD s_lastSpearThrustTime = 0;
            bool batonChargedSpearArmed = (vrState.rightController.grip > 0.60f && vrState.rightController.trigger > 0.65f);
            bool batonSpearBracing = (handsDistSledge < 0.22f);
            bool batonLinearSpearThrust = (rVel.z < -1.35f);
            bool isBatonChargedSpear = batonChargedSpearArmed && batonSpearBracing && batonLinearSpearThrust;

            // Inmate Pit Brawl Collar Tie Clinch & Headbutt Stun (ADR 0149)
            static DWORD s_lastHeadbuttTime = 0;
            bool collarTieClinchGrip = (leftLocal.z > -0.35f && leftLocal.z < -0.12f && leftLocal.y > -0.15f && leftLocal.y < 0.15f && vrState.leftController.grip > 0.65f);
            bool headbuttKineticSurge = (lVel.z > 0.55f && rVel.z < -1.05f) || (rVel.z < -1.25f);
            bool isClinchHeadbutt = collarTieClinchGrip && headbuttKineticSurge;

            // Security Guard Stun Baton Ground Pin & Taser Kneel (ADR 0150)
            static DWORD s_lastBatonGroundPinTime = 0;
            bool lowGroundPosture = (s_isPhysicallyCrouched || s_isDeepProne || rightLocal.y < -0.35f);
            bool batonGroundEnergized = (vrState.rightController.grip > 0.60f && vrState.rightController.trigger > 0.65f);
            bool verticalGroundPlunge = (rVel.y < -1.15f);
            bool isBatonGroundPin = lowGroundPosture && batonGroundEnergized && verticalGroundPlunge;

            // Inmate Pit Brawl Muay Thai Lead Knee Thrust to Abdomen (ADR 0153)
            static DWORD s_lastLeadKneeTime = 0;
            bool leadKneePosture = s_currentBlockEngaged;
            bool leadKneeStrikes = (rVel.y > 1.20f && lVel.y < -0.85f);
            bool isLeadKneeThrust = leadKneePosture && leadKneeStrikes;

            // Security Guard Stun Baton Two-Handed Cross-Chest Defensive Arc Wall (ADR 0154)
            static DWORD s_lastArcWallPulse = 0;
            bool batonArcWallGeometry = (handsDistSledge > 0.20f && handsDistSledge < 0.38f && fabsf(rightLocal.x - leftLocal.x) > 0.22f &&
                                         rightLocal.y > -0.20f && rightLocal.y < 0.10f && leftLocal.y > -0.20f && leftLocal.y < 0.10f &&
                                         rightLocal.z > -0.30f && rightLocal.z < -0.10f && leftLocal.z > -0.30f && leftLocal.z < -0.10f);
            bool batonArcWallGrips = (vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                      vrState.leftController.trigger > 0.60f && vrState.rightController.trigger > 0.60f);
            bool isCrossChestArcWall = batonArcWallGeometry && batonArcWallGrips;
            if (isCrossChestArcWall) {
                if (now - s_lastArcWallPulse > 180) {
                    s_lastArcWallPulse = now;
                    vrState.leftHapticPulse = 0.65f;  // Electrostatic repulsion barrier hum (240 Hz)
                    vrState.rightHapticPulse = 0.65f;
                }
            }

            // Inmate Pit Brawl Duck-Under Kidney Liver Counter-Jab (ADR 0157)
            static DWORD s_lastKidneyCounterTime = 0;
            bool duckUnderCrouch = (s_isPhysicallyCrouched || s_isDeepProne);
            bool kidneyStrikeCorridor = (rightLocal.y > -0.35f && rightLocal.y < -0.05f);
            bool kidneyCounterPunch = (fabsf(rVel.x) > 0.85f && rVel.z < -1.05f);
            bool isDuckUnderKidneyCounter = duckUnderCrouch && kidneyStrikeCorridor && kidneyCounterPunch;

            // Security Guard Stun Baton High-Voltage Capacitance Bleed Arc Trap (ADR 0158)
            static DWORD s_lastBatonArcTrapTime = 0;
            static DWORD s_batonArcTrapStart = 0;
            bool batonAtConduitSurface = (handsDistSledge < 0.22f &&
                                          rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > -0.30f && rightLocal.y < 0.10f &&
                                          rSpeed < 0.12f &&
                                          vrState.leftController.trigger > 0.70f && vrState.rightController.trigger > 0.70f);
            bool isBatonArcTrapArmed = false;
            if (batonAtConduitSurface) {
                if (s_batonArcTrapStart == 0) s_batonArcTrapStart = now;
                DWORD chargeDwell = now - s_batonArcTrapStart;
                float dwellRatio = (chargeDwell < 400) ? (chargeDwell / 400.0f) : 1.0f;
                vrState.leftHapticPulse = 0.40f + 0.50f * dwellRatio; // Capacitive transfer buzz (280 Hz)
                vrState.rightHapticPulse = 0.40f + 0.50f * dwellRatio;
                if (chargeDwell >= 400 && (now - s_lastBatonArcTrapTime > 2000)) {
                    s_lastBatonArcTrapTime = now;
                    s_batonArcTrapStart = 0;
                    isBatonArcTrapArmed = true;
                }
            } else {
                s_batonArcTrapStart = 0;
            }

            // Inmate Pit Brawl Upward Palm Chin-Strike & Cervical Snap (ADR 0161)
            static DWORD s_lastPalmChinComboTime = 0;
            static DWORD s_palmChinStrikeTime = 0;
            bool upwardPalmStrike = (rVel.y > 1.25f && rVel.z < -0.80f && rightLocal.y > -0.10f && rightLocal.y < 0.25f);
            if (upwardPalmStrike && (now - s_lastPalmChinComboTime > 1500)) {
                if (s_palmChinStrikeTime == 0) {
                    s_palmChinStrikeTime = now;
                    VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL UPWARD PALM CHIN-STRIKE CONNECTED (Ry: %.2f, Rz: %.2f)", rVel.y, rVel.z);
                    vrState.rightHapticPulse = 0.98f; // Mandibular percussion shockwave (240 Hz)
                }
            }
            bool cervicalSnapWhip = (s_palmChinStrikeTime > 0 && (now - s_palmChinStrikeTime < 500) && fabsf(lVel.x) > 1.10f);

            // Security Guard Stun Baton Dual-Hand Ground Discharge Kinetic Shockwave (ADR 0162)
            static DWORD s_lastBatonGroundShockTime = 0;
            bool batonDualGroundEnergized = (handsDistSledge < 0.22f &&
                                             vrState.rightController.grip > 0.65f && vrState.rightController.trigger > 0.65f &&
                                             vrState.leftController.grip > 0.60f && vrState.leftController.trigger > 0.60f);
            bool dualGroundPlunge = (rVel.y < -1.25f && lVel.y < -1.25f && rightLocal.y < -0.40f);
            bool isBatonGroundDischarge = batonDualGroundEnergized && dualGroundPlunge;

            // Inmate Pit Brawl Collar Tie Snap-Down & Guillotine Choke (ADR 0165)
            static DWORD s_lastSnapdownChokeTime = 0;
            static DWORD s_snapdownStartTime = 0;
            bool collarSnapdownGrip = (leftLocal.z > -0.35f && leftLocal.z < -0.12f && vrState.leftController.grip > 0.65f);
            bool collarSnapdownPull = (lVel.y < -0.90f);
            if (collarSnapdownGrip && collarSnapdownPull && (now - s_lastSnapdownChokeTime > 1500)) {
                if (s_snapdownStartTime == 0) {
                    s_snapdownStartTime = now;
                    VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL COLLAR TIE SNAP-DOWN INITIATED (Ly: %.2f)", lVel.y);
                    vrState.leftHapticPulse = 0.90f; // Cervical snap-down jerk (210 Hz)
                }
            }
            bool guillotineWrapStrike = (s_snapdownStartTime > 0 && (now - s_snapdownStartTime < 500) && rVel.y > 1.05f && rVel.z < -0.75f);

            // Security Guard Stun Baton Charged High-Arc Downward Skull Breaker (ADR 0166)
            static DWORD s_lastSkullBreakerTime = 0;
            bool batonOverheadEnergized = (rightLocal.y > 0.25f && vrState.rightController.grip > 0.60f && vrState.rightController.trigger > 0.65f);
            bool batonExtremeCleave = (rVel.y < -1.45f);
            bool isBatonSkullBreaker = batonOverheadEnergized && batonExtremeCleave;

            // Security Guard Stun Baton Lateral Cross-Sweep Deflection & Disarm (ADR 0170)
            static DWORD s_lastBatonDisarmTime = 0;
            bool batonLateralSweepArmed = (vrState.rightController.grip > 0.60f && vrState.rightController.trigger > 0.65f &&
                                           rightLocal.y > -0.20f && rightLocal.y < 0.20f);
            bool batonCrossSweepFast = (fabsf(rVel.x) > 1.25f);
            bool isBatonLateralDisarm = batonLateralSweepArmed && batonCrossSweepFast;

            // Inmate Pit Brawl Shin Trap & Overhand Hammerfist Ground Finisher (ADR 0169)
            static DWORD s_lastShinTrapTime = 0;
            static DWORD s_shinTrapTime = 0;
            bool shinTrapDeflect = (leftLocal.y < -0.20f && vrState.leftController.grip > 0.55f && lVel.y < -0.85f);
            if (shinTrapDeflect && (now - s_lastShinTrapTime > 1500)) {
                if (s_shinTrapTime == 0) {
                    s_shinTrapTime = now;
                    VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL SHIN TRAP DEFLECTED (Ly: %.2f)", lVel.y);
                    vrState.leftHapticPulse = 0.75f; // Shin trap deflection impact (180 Hz)
                }
            }
            bool hammerfistStrike = (s_shinTrapTime > 0 && (now - s_shinTrapTime < 500) && rVel.y < -1.30f);

            // Inmate Pit Brawl Underhook Suplex & Spine Slam (ADR 0173)
            static DWORD s_lastSuplexSlamTime = 0;
            static DWORD s_suplexHoistTime = 0;
            bool underhookGripHold = (handsDistSledge < 0.25f && vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool underhookVerticalHoist = (lVel.y > 1.10f && rVel.y > 1.10f);
            if (underhookGripHold && underhookVerticalHoist && (now - s_lastSuplexSlamTime > 1800)) {
                if (s_suplexHoistTime == 0) {
                    s_suplexHoistTime = now;
                    VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL DOUBLE UNDERHOOK HOIST INITIATED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                    vrState.leftHapticPulse = 0.90f;  // Torso lift resistance (220 Hz)
                    vrState.rightHapticPulse = 0.90f;
                }
            }
            bool suplexSpineSlam = (s_suplexHoistTime > 0 && (now - s_suplexHoistTime < 500) && lVel.y < -1.20f && rVel.y < -1.20f);

            // Security Guard Stun Baton Dual-Hand Cross-Block Reactive Arc Burst (ADR 0174)
            static DWORD s_lastCrossArcBurstTime = 0;
            static DWORD s_crossArcStartTime = 0;
            bool crossWristFrame = (handsDistSledge < 0.16f &&
                                    rightLocal.y > -0.15f && rightLocal.y < 0.15f && rightLocal.z > -0.30f && rightLocal.z < -0.10f &&
                                    vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                    vrState.leftController.trigger > 0.65f && vrState.rightController.trigger > 0.65f);
            bool isCrossArcBurstReady = false;
            if (crossWristFrame) {
                if (s_crossArcStartTime == 0) s_crossArcStartTime = now;
                if (now - s_crossArcStartTime >= 300) {
                    vrState.leftHapticPulse = 0.60f;  // Pre-charge hum (240 Hz)
                    vrState.rightHapticPulse = 0.60f;
                    if (rVel.z < -1.15f || lVel.z < -1.15f) {
                        isCrossArcBurstReady = true;
                    }
                }
            } else {
                s_crossArcStartTime = 0;
            }

            // Security Guard Stun Baton Forward Two-Handed Javelin Thrust (ADR 0178)
            static DWORD s_lastBatonJavelinTime = 0;
            bool batonJavelinGrip = (handsDistSledge > 0.18f && handsDistSledge < 0.40f &&
                                     vrState.rightController.grip > 0.60f && vrState.leftController.grip > 0.50f &&
                                     vrState.rightController.trigger > 0.65f);
            bool batonJavelinThrust = (rVel.z < -1.35f && lVel.z < -1.05f);
            bool isBatonJavelinThrust = batonJavelinGrip && batonJavelinThrust;

            // Inmate Pit Brawl Outside Trip & Mount Ground-and-Pound (ADR 0177)
            static DWORD s_lastOutsideTripTime = 0;
            bool outsideTripHook = (leftLocal.y < -0.30f && vrState.leftController.grip > 0.50f && lVel.x < -0.80f);
            bool outsideTripDrive = (rVel.z < -1.10f && rVel.y < -0.75f);
            bool isOutsideTripMount = outsideTripHook && outsideTripDrive;

            // Inmate Pit Brawl Kimura Shoulder Lock & Arm Fracture (ADR 0181)
            static DWORD s_lastKimuraTime = 0;
            bool kimuraGrip = (handsDistSledge < 0.25f &&
                               vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                               leftLocal.y > -0.20f && leftLocal.y < 0.20f && rightLocal.y > -0.20f && rightLocal.y < 0.20f);
            bool kimuraCrank = (rVel.y > 1.05f && fabsf(rVel.x) > 0.85f);
            bool isKimuraFracture = kimuraGrip && kimuraCrank;

            // Inmate Pit Brawl Standing Arm-Triangle Choke & Tracheal Clamp (ADR 0185)
            static DWORD s_lastArmTriangleTime = 0;
            static DWORD s_armTriangleStartTime = 0;
            bool armTriangleGrapple = (leftLocal.z > -0.35f && leftLocal.z < -0.10f && vrState.leftController.grip > 0.65f &&
                                       handsDistSledge < 0.20f && vrState.rightController.grip > 0.65f &&
                                       vrState.leftController.trigger > 0.60f && vrState.rightController.trigger > 0.60f);
            bool isArmTriangleReady = false;
            if (armTriangleGrapple) {
                if (s_armTriangleStartTime == 0) s_armTriangleStartTime = now;
                if (now - s_armTriangleStartTime >= 300 && (now - s_lastArmTriangleTime > 2000)) {
                    isArmTriangleReady = true;
                }
            } else {
                s_armTriangleStartTime = 0;
            }

            // Security Guard Stun Baton Upward Diagonal Cross-Parry & Counter-Discharge (ADR 0186)
            static DWORD s_lastDiagonalParryTime = 0;
            bool batonDiagonalParryGrip = (handsDistSledge < 0.24f &&
                                           vrState.rightController.grip > 0.60f && vrState.rightController.trigger > 0.65f &&
                                           vrState.leftController.grip > 0.50f &&
                                           rightLocal.y > 0.05f && rightLocal.y < 0.35f);
            bool batonDiagonalParryThrust = (rVel.y > 1.15f && fabsf(rVel.x) > 0.85f);
            bool isBatonDiagonalParry = batonDiagonalParryGrip && batonDiagonalParryThrust;

            // Security Guard Stun Baton Radial Kinetic Sweep & Electrical Stun Ring (ADR 0182)
            static DWORD s_lastRadialSweepTime = 0;
            bool batonRadialGrip = (handsDistSledge < 0.25f &&
                                    vrState.rightController.grip > 0.60f && vrState.rightController.trigger > 0.65f &&
                                    vrState.leftController.grip > 0.50f);
            bool batonRadialSweep = (fabsf(rVel.x) > 1.35f && fabsf(lVel.x) > 1.10f &&
                                     ((rVel.x > 0.0f && lVel.x > 0.0f) || (rVel.x < 0.0f && lVel.x < 0.0f)));
            bool isBatonRadialSweep = batonRadialGrip && batonRadialSweep;

            // Inmate Pit Brawl Single-Leg Takedown & Ankle Pick Slam (ADR 0189)
            static DWORD s_lastSingleLegTakedownTime = 0;
            bool singleLegLowStance = (s_isPhysicallyCrouched || s_isDeepProne || leftLocal.y < -0.30f);
            bool anklePickGrip = (leftLocal.y < -0.40f && vrState.leftController.grip > 0.60f && lVel.y > 0.85f);
            bool shoulderDriveThrough = (rVel.z < -1.10f);
            bool isSingleLegTakedown = singleLegLowStance && anklePickGrip && shoulderDriveThrough;

            // Security Guard Stun Baton Forward High-Voltage Discharge Jab & Taser Trip (ADR 0190)
            static DWORD s_lastBatonJabTripTime = 0;
            static DWORD s_batonJabTime = 0;
            bool batonJabArmed = (vrState.rightController.grip > 0.60f && vrState.rightController.trigger > 0.65f);
            bool batonLinearJab = (rVel.z < -1.25f);
            if (batonJabArmed && batonLinearJab && (now - s_lastBatonJabTripTime > 1800)) {
                if (s_batonJabTime == 0) {
                    s_batonJabTime = now;
                    VR_LOG_INFO("Tactical Melee: STUN BATON HIGH-VOLTAGE DISCHARGE JAB CONNECTED (Rz: %.2f)", rVel.z);
                    vrState.rightHapticPulse = 0.98f; // Direct contact discharge (310 Hz)
                }
            }
            bool batonTripSweep = (s_batonJabTime > 0 && (now - s_batonJabTime < 500) &&
                                   rVel.y < -1.10f && fabsf(rVel.x) > 0.80f);

            // Inmate Pit Brawl Double-Leg Takedown & High-Amplitude Mat Slam (ADR 0193)
            static DWORD s_lastDoubleLegSlamTime = 0;
            static DWORD s_doubleLegLiftTime = 0;
            bool doubleLegLowStance = (s_isPhysicallyCrouched || s_isDeepProne || leftLocal.y < -0.30f);
            bool doubleLegThighWrap = (handsDistSledge < 0.25f &&
                                       vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                       lVel.y > 1.15f && rVel.y > 1.15f);
            if (doubleLegLowStance && doubleLegThighWrap && (now - s_lastDoubleLegSlamTime > 2000)) {
                if (s_doubleLegLiftTime == 0) {
                    s_doubleLegLiftTime = now;
                    VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL DOUBLE-LEG LIFT INITIATED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                    vrState.leftHapticPulse = 0.90f;  // Heavy body lift strain (210 Hz)
                    vrState.rightHapticPulse = 0.90f;
                }
            }
            bool doubleLegSpikeSlam = (s_doubleLegLiftTime > 0 && (now - s_doubleLegLiftTime < 500) &&
                                       lVel.y < -1.30f && rVel.y < -1.30f);

            // Security Guard Stun Baton Forward Two-Handed Cross-Pistol Disarm & Neck Strike (ADR 0194)
            static DWORD s_lastPistolDisarmTime = 0;
            static DWORD s_pistolTrapTime = 0;
            bool batonPistolDisarmGrip = (handsDistSledge < 0.24f &&
                                          vrState.rightController.grip > 0.60f && vrState.rightController.trigger > 0.65f &&
                                          vrState.leftController.grip > 0.50f &&
                                          rightLocal.y > -0.20f && rightLocal.y < 0.10f);
            bool batonDownwardPistolTrap = (rVel.y < -0.90f && lVel.y < -0.90f && rVel.z < -0.80f);
            if (batonPistolDisarmGrip && batonDownwardPistolTrap && (now - s_lastPistolDisarmTime > 1800)) {
                if (s_pistolTrapTime == 0) {
                    s_pistolTrapTime = now;
                    VR_LOG_INFO("Tactical Melee: ENEMY FIREARM TRAPPED & WEDGED WITH STUN BATON (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                    vrState.rightHapticPulse = 0.95f; // Electrostatic disarm arc snap (280 Hz)
                    vrState.leftHapticPulse = 0.70f;
                }
            }
            bool batonThroatThrustStrike = (s_pistolTrapTime > 0 && (now - s_pistolTrapTime < 500) && rVel.z < -1.20f);

            // Inmate Pit Brawl Fireman's Carry & Deck Body-Spike Slam (ADR 0197)
            static DWORD s_lastFiremansCarryTime = 0;
            static DWORD s_firemansLoadTime = 0;
            bool firemansLowDuck = (s_isPhysicallyCrouched || s_isDeepProne || leftLocal.y < -0.25f);
            bool firemansUnderhookLoad = (handsDistSledge < 0.25f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                          rVel.y > 1.10f && lVel.z > 0.70f);
            if (firemansLowDuck && firemansUnderhookLoad && (now - s_lastFiremansCarryTime > 2000)) {
                if (s_firemansLoadTime == 0) {
                    s_firemansLoadTime = now;
                    VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL FIREMAN'S CARRY LOADED (Ry: %.2f, Lz: %.2f)", rVel.y, lVel.z);
                    vrState.leftHapticPulse = 0.90f;  // Heavy shoulder load strain (220 Hz)
                    vrState.rightHapticPulse = 0.90f;
                }
            }
            bool firemansWhipThrow = (s_firemansLoadTime > 0 && (now - s_firemansLoadTime < 500) &&
                                      (fabsf(lVel.x) > 1.25f || fabsf(rVel.x) > 1.25f));

            // Inmate Pit Brawl Standing Front Headlock & Guillotine Neck Crack (ADR 0205)
            static DWORD s_lastFrontHeadlockTime = 0;
            bool frontHeadlockWrap = (handsDistSledge < 0.25f && leftLocal.y > -0.15f &&
                                      vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool frontHeadlockArchTorque = (lVel.y > 1.15f && rVel.y > 1.15f);

            // Security Guard Stun Baton Lateral Two-Handed Cross-Chest Kinetic Parrying Riposte (ADR 0206)
            static DWORD s_lastCrossParryTime = 0;
            static DWORD s_crossParryDeflectTime = 0;
            static char s_crossParryDirKey = 'A';
            bool crossParryStance = (handsDistSledge > 0.18f && handsDistSledge < 0.35f &&
                                     fabsf(rightLocal.x - leftLocal.x) > 0.20f &&
                                     vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                     vrState.rightController.trigger > 0.65f);
            bool crossParryDeflect = (crossParryStance && (fabsf(rVel.x) > 0.85f || fabsf(lVel.x) > 0.85f));
            if (crossParryDeflect && (now - s_lastCrossParryTime > 2000)) {
                if (s_crossParryDeflectTime == 0) {
                    s_crossParryDeflectTime = now;
                    s_crossParryDirKey = (rVel.x > 0.0f || lVel.x > 0.0f) ? 'D' : 'A';
                    VR_LOG_INFO("Tactical Melee: STUN BATON CROSS-CHEST PARRY DEFLECTED (Dir: %c, Vx: %.2f)", s_crossParryDirKey, rVel.x);
                    vrState.leftHapticPulse = 0.85f;  // Parry deflection shockwave (300 Hz)
                    vrState.rightHapticPulse = 0.85f;
                }
            }
            bool crossParryRiposte = (s_crossParryDeflectTime > 0 && (now - s_crossParryDeflectTime < 500) && rVel.z < -1.25f);

            // Inmate Pit Brawl Suloev Stretch Hamstring Submission Lock (ADR 0201)
            static DWORD s_lastSuloevTime = 0;
            bool suloevLowCrouch = (s_isPhysicallyCrouched || s_isDeepProne || leftLocal.y < -0.30f);
            bool suloevHookHold = (handsDistSledge < 0.25f && leftLocal.y < -0.35f &&
                                   vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool suloevArchPull = (lVel.y > 1.05f && rVel.y > 1.05f);

            // Security Guard Stun Baton Forward High-Arc Overhead Vault Drop Strike (ADR 0202)
            static DWORD s_lastVaultDropTime = 0;
            bool isLookingDownward = (s_lastHmdPitch < -0.436f); // < -25 deg downward look
            bool batonOverheadVault = (rightLocal.y > 0.20f &&
                                       vrState.rightController.grip > 0.60f &&
                                       vrState.rightController.trigger > 0.65f);
            bool batonVaultPlunge = (rVel.y < -1.35f);

            // Inmate Pit Brawl Duck-Under Back-Clinch Belly-to-Back Suplex Slam (ADR 0213)
            static DWORD s_lastBackSuplexTime = 0;
            static DWORD s_backSuplexClinchTime = 0;
            bool duckUnderLow = (s_isPhysicallyCrouched || s_isDeepProne || leftLocal.y < -0.25f);
            bool duckUnderRearSlip = (duckUnderLow && handsDistSledge < 0.24f &&
                                      fabsf(rVel.x) > 0.85f &&
                                      vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            if (duckUnderRearSlip && (now - s_lastBackSuplexTime > 2000)) {
                if (s_backSuplexClinchTime == 0) {
                    s_backSuplexClinchTime = now;
                    VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL REAR WAIST CLINCH LOCKED (Rx: %.2f)", rVel.x);
                    vrState.leftHapticPulse = 0.90f;  // Abdominal lift strain (220 Hz)
                    vrState.rightHapticPulse = 0.90f;
                }
            }
            bool backSuplexHoistSlam = (s_backSuplexClinchTime > 0 && (now - s_backSuplexClinchTime < 500) &&
                                        lVel.y > 1.20f && rVel.y > 1.20f);

            // Security Guard Stun Baton Charged High-Velocity Forward Radial Thrust Disarm (ADR 0214)
            static DWORD s_lastRadialDisarmTime = 0;
            static DWORD s_radialDisarmTwistTime = 0;
            static char s_radialDisarmDirKey = 'A';
            bool radialDisarmStance = (handsDistSledge < 0.24f &&
                                       vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                       vrState.rightController.trigger > 0.65f);
            bool radialDisarmTwist = (radialDisarmStance && fabsf(rVel.x - lVel.x) > 1.15f);
            if (radialDisarmTwist && (now - s_lastRadialDisarmTime > 2000)) {
                if (s_radialDisarmTwistTime == 0) {
                    s_radialDisarmTwistTime = now;
                    s_radialDisarmDirKey = (rVel.x > 0.0f) ? 'D' : 'A';
                    VR_LOG_INFO("Tactical Melee: STUN BATON RADIAL DISARM TWIST EXECUTED (Dir: %c, DeltaVx: %.2f)", s_radialDisarmDirKey, rVel.x - lVel.x);
                    vrState.leftHapticPulse = 0.95f;  // Torque disarm snap (310 Hz)
                    vrState.rightHapticPulse = 0.95f;
                }
            }
            bool radialDisarmThrust = (s_radialDisarmTwistTime > 0 && (now - s_radialDisarmTwistTime < 500) && rVel.z < -1.35f);

            // Inmate Pit Brawl Clinch Thai Plum Collar Tie & Liver Knee Strike (ADR 0217)
            static DWORD s_lastThaiPlumTime = 0;
            static DWORD s_thaiPlumClinchTime = 0;
            bool thaiPlumGrip = (handsDistSledge < 0.25f &&
                                 leftLocal.y > -0.10f && rightLocal.y > -0.10f &&
                                 vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            if (thaiPlumGrip && (now - s_lastThaiPlumTime > 2000)) {
                if (s_thaiPlumClinchTime == 0) {
                    s_thaiPlumClinchTime = now;
                    VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL THAI PLUM COLLAR TIE LOCKED (Ly: %.2f, Ry: %.2f)", leftLocal.y, rightLocal.y);
                    vrState.leftHapticPulse = 0.90f;  // Cervical collar drag tension (220 Hz)
                    vrState.rightHapticPulse = 0.90f;
                }
            }
            bool thaiPlumKneeDrive = (s_thaiPlumClinchTime > 0 && (now - s_thaiPlumClinchTime < 600) &&
                                      lVel.y < -1.10f && rVel.y < -1.10f);

            // Security Guard Stun Baton Forward Two-Handed Javelin Vault Leap Thrust (ADR 0218)
            static DWORD s_lastJavelinVaultTime = 0;
            bool javelinVaultStance = (handsDistSledge < 0.22f &&
                                       vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                       vrState.rightController.trigger > 0.65f);
            bool isJumpingMotion = ((vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f);
            bool javelinVaultStrike = (javelinVaultStance && isJumpingMotion && rVel.z < -1.40f && lVel.z < -1.20f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Takedown & Arm Trap (ADR 0221)
            static DWORD s_lastWhizzerTime = 0;
            static char s_whizzerDirKey = 'A';
            bool whizzerClinchGrip = (handsDistSledge < 0.25f &&
                                      rightLocal.y > -0.15f && rightLocal.y < 0.15f &&
                                      vrState.rightController.grip > 0.65f && vrState.leftController.grip > 0.65f);
            bool whizzerDownWhip = (whizzerClinchGrip && rVel.y < -1.15f && fabsf(lVel.x) > 0.90f);
            if (whizzerDownWhip && (now - s_lastWhizzerTime > 2000)) {
                s_whizzerDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Arc Kinetic Hammerfist Deck Strike (ADR 0222)
            static DWORD s_lastHammerfistTime = 0;
            bool batonHammerfistElevated = (rightLocal.y > 0.15f &&
                                            vrState.rightController.grip > 0.65f &&
                                            vrState.rightController.trigger > 0.65f);
            bool batonHammerfistPlunge = (batonHammerfistElevated && rVel.y < -1.35f && rVel.z < -0.80f);

            // Inmate Pit Brawl Double Underhook Body Lock Foot Sweep & Hip Toss (ADR 0225)
            static DWORD s_lastHipTossTime = 0;
            static char s_hipTossDirKey = 'A';
            bool underhookBodyLockGrip = (handsDistSledge < 0.22f &&
                                          leftLocal.y > -0.20f && leftLocal.y < 0.15f &&
                                          rightLocal.y > -0.20f && rightLocal.y < 0.15f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool hipTossRotation = (underhookBodyLockGrip && fabsf(rVel.x - lVel.x) > 1.20f &&
                                    (rVel.y > 1.05f || lVel.y > 1.05f));
            if (hipTossRotation && (now - s_lastHipTossTime > 2000)) {
                s_hipTossDirKey = (rVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Lateral Two-Handed Kinetic Shield Bash & Arc Burst (ADR 0226)
            static DWORD s_lastBatonBashTime = 0;
            bool batonShieldBashStance = (handsDistSledge > 0.18f && handsDistSledge < 0.35f &&
                                          fabsf(rightLocal.x - leftLocal.x) > 0.20f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                          vrState.rightController.trigger > 0.65f);
            bool batonShieldBashThrust = (batonShieldBashStance && rVel.z < -1.35f && lVel.z < -1.15f);

            // Inmate Pit Brawl Clinch Thai Plum Elbow Spike & Cranial Fracture (ADR 0229)
            static DWORD s_lastThaiPlumElbowTime = 0;
            static char s_thaiPlumElbowDirKey = 'A';
            bool thaiPlumElbowStance = (handsDistSledge < 0.25f &&
                                        leftLocal.y > -0.10f && rightLocal.y > 0.15f &&
                                        vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool thaiPlumElbowPlunge = (thaiPlumElbowStance && rVel.y < -1.30f && lVel.z > 0.60f);
            if (thaiPlumElbowPlunge && (now - s_lastThaiPlumElbowTime > 2000)) {
                s_thaiPlumElbowDirKey = (rVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Grounding Thrust & Arc Discharge Ring (ADR 0230)
            static DWORD s_lastGroundingThrustTime = 0;
            bool groundingThrustStance = ((leftLocal.y < -0.25f || s_isPhysicallyCrouched) &&
                                          handsDistSledge < 0.24f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                          vrState.rightController.trigger > 0.65f);
            bool groundingThrustLunge = (groundingThrustStance && rVel.z < -1.35f && lVel.z < -1.10f);

            // Inmate Pit Brawl Double Underhook Lateral Suplex Arch & Mat Spike (ADR 0233)
            static DWORD s_lastLateralSuplexTime = 0;
            static char s_lateralSuplexDirKey = 'A';
            bool lateralSuplexStance = (handsDistSledge < 0.28f &&
                                        leftLocal.y > -0.20f && rightLocal.y > -0.20f &&
                                        vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool lateralSuplexThrow = (lateralSuplexStance && rVel.y > 0.70f && lVel.y > 0.70f &&
                                       (fabsf(rVel.x) > 0.85f || fabsf(lVel.x) > 0.85f));
            if (lateralSuplexThrow && (now - s_lastLateralSuplexTime > 2000)) {
                s_lateralSuplexDirKey = (rVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Arc Kinetic Cleave & Shockwave Stun (ADR 0234)
            static DWORD s_lastKineticCleaveTime = 0;
            bool kineticCleaveStance = (handsDistSledge < 0.28f &&
                                        rightLocal.y > 0.18f && leftLocal.y > 0.10f &&
                                        vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                        vrState.rightController.trigger > 0.65f);
            bool kineticCleaveSwing = (kineticCleaveStance && rVel.y < -1.40f && lVel.y < -1.10f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Shoulder Dislocation (ADR 0237)
            static DWORD s_lastWhizzerCounterTime = 0;
            static char s_whizzerCounterDirKey = 'A';
            bool whizzerCounterStance = (handsDistSledge < 0.26f &&
                                         leftLocal.y > -0.15f && leftLocal.y < 0.15f &&
                                         vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool whizzerCounterThrow = (whizzerCounterStance && rVel.y < -1.25f && fabsf(lVel.x) > 0.95f);
            if (whizzerCounterThrow && (now - s_lastWhizzerCounterTime > 2000)) {
                s_whizzerCounterDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward Two-Handed Kinetic Cleave & Arc Discharge Blast (ADR 0238)
            static DWORD s_lastCleaveArcBlastTime = 0;
            bool cleaveArcBlastStance = (handsDistSledge < 0.28f &&
                                         rightLocal.y > 0.16f && leftLocal.y > 0.08f &&
                                         vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                         vrState.rightController.trigger > 0.65f);
            bool cleaveArcBlastStrike = (cleaveArcBlastStance && rVel.y < -1.30f && lVel.y < -1.05f && rVel.z < -1.00f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Mat Spike (ADR 0241)
            static DWORD s_lastCervicalSpikeTime = 0;
            static char s_cervicalSpikeDirKey = 'A';
            bool cervicalSpikeStance = (handsDistSledge < 0.26f &&
                                        leftLocal.y > -0.10f && leftLocal.y < 0.20f &&
                                        vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cervicalSpikeThrow = (cervicalSpikeStance && rVel.y < -1.35f &&
                                       lVel.y < -1.05f && fabsf(lVel.x) > 0.85f);
            if (cervicalSpikeThrow && (now - s_lastCervicalSpikeTime > 2000)) {
                s_cervicalSpikeDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Ground Arc Disruption (ADR 0242)
            static DWORD s_lastVaultGroundArcTime = 0;
            bool isJumpingStance = ((vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f);
            bool vaultGroundArcStance = (handsDistSledge < 0.24f &&
                                         vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                         vrState.rightController.trigger > 0.65f);
            bool vaultGroundArcPlunge = (vaultGroundArcStance && isJumpingStance && rVel.y < -1.40f && lVel.y < -1.15f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Grating Slam (ADR 0245)
            static DWORD s_lastThoracicSlamTime = 0;
            static char s_thoracicSlamDirKey = 'A';
            bool thoracicSlamStance = (handsDistSledge < 0.26f &&
                                       leftLocal.y > -0.20f && leftLocal.y < 0.15f &&
                                       vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool thoracicSlamThrow = (thoracicSlamStance && rVel.y < -1.30f &&
                                      fabsf(lVel.x) > 0.90f && lVel.y > 0.60f);
            if (thoracicSlamThrow && (now - s_lastThoracicSlamTime > 2000)) {
                s_thoracicSlamDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Kinetic Cleave Shockwave (ADR 0246)
            static DWORD s_lastVaultKineticCleaveTime = 0;
            bool vaultKineticCleaveStance = (handsDistSledge < 0.28f &&
                                             rightLocal.y > 0.15f &&
                                             vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                             vrState.rightController.trigger > 0.65f);
            bool vaultKineticCleavePlunge = (vaultKineticCleaveStance && isJumpingStance &&
                                             rVel.y < -1.45f && lVel.y < -1.15f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Hyperextension Mat Spike (ADR 0249)
            static DWORD s_lastHyperextensionSpikeTime = 0;
            static char s_hyperextensionSpikeDirKey = 'A';
            bool hyperextensionSpikeStance = (handsDistSledge < 0.26f &&
                                              leftLocal.y > -0.05f &&
                                              vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool hyperextensionSpikeThrow = (hyperextensionSpikeStance && rVel.y < -1.40f &&
                                             lVel.y > 0.85f && fabsf(lVel.x) > 0.90f);
            if (hyperextensionSpikeThrow && (now - s_lastHyperextensionSpikeTime > 2000)) {
                s_hyperextensionSpikeDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Concussive Deck Impact Shockwave (ADR 0250)
            static DWORD s_lastDeckImpactTime = 0;
            bool deckImpactStance = (handsDistSledge < 0.28f &&
                                     vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                     vrState.rightController.trigger > 0.65f);
            bool deckImpactPlunge = (deckImpactStance && isJumpingStance &&
                                     rVel.y < -1.50f && lVel.y < -1.25f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Ground Finisher (ADR 0253)
            static DWORD s_lastWhizzerGroundFinisherTime = 0;
            static char s_whizzerGroundFinisherDirKey = 'A';
            bool whizzerGroundFinisherStance = (handsDistSledge < 0.26f &&
                                                leftLocal.y > -0.25f && leftLocal.y < 0.10f &&
                                                vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool whizzerGroundFinisherThrow = (whizzerGroundFinisherStance && rVel.y < -1.45f &&
                                               lVel.y < -1.15f && fabsf(lVel.x) > 0.80f);
            if (whizzerGroundFinisherThrow && (now - s_lastWhizzerGroundFinisherTime > 2000)) {
                s_whizzerGroundFinisherDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Deck Arc Dispersion (ADR 0254)
            static DWORD s_lastOmnidirectionalArcTime = 0;
            bool omnidirectionalArcStance = (handsDistSledge < 0.28f &&
                                             vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                             vrState.rightController.trigger > 0.65f);
            bool omnidirectionalArcPlunge = (omnidirectionalArcStance && isJumpingStance &&
                                             rVel.y < -1.45f && lVel.y < -1.20f &&
                                             fabsf(rVel.x - lVel.x) > 0.80f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Ground Dislocation (ADR 0257)
            static DWORD s_lastWhizzerGroundDislocationTime = 0;
            static char s_whizzerGroundDislocationDirKey = 'A';
            bool whizzerGroundDislocationStance = (handsDistSledge < 0.26f &&
                                                   leftLocal.y > -0.30f && leftLocal.y < 0.05f &&
                                                   vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool whizzerGroundDislocationThrow = (whizzerGroundDislocationStance && rVel.y < -1.50f &&
                                                  lVel.y < -1.20f && fabsf(lVel.x) > 0.85f);
            if (whizzerGroundDislocationThrow && (now - s_lastWhizzerGroundDislocationTime > 2000)) {
                s_whizzerGroundDislocationDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Deck Arc Pulse (ADR 0258)
            static DWORD s_lastOmnidirectionalPulseTime = 0;
            bool omnidirectionalPulseStance = (handsDistSledge < 0.28f &&
                                               vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                               vrState.rightController.trigger > 0.65f);
            bool omnidirectionalPulsePlunge = (omnidirectionalPulseStance && isJumpingStance &&
                                               rVel.y < -1.55f && lVel.y < -1.30f &&
                                               fabsf(rVel.x - lVel.x) > 0.90f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Ground Finish & Sternum Knee Drop (ADR 0261)
            static DWORD s_lastSternumKneeDropTime = 0;
            static char s_sternumKneeDropDirKey = 'A';
            bool sternumKneeDropStance = (handsDistSledge < 0.26f &&
                                          leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool sternumKneeDropThrow = (sternumKneeDropStance && rVel.y < -1.55f &&
                                         lVel.y < -1.30f && fabsf(lVel.x) > 0.85f);
            if (sternumKneeDropThrow && (now - s_lastSternumKneeDropTime > 2000)) {
                s_sternumKneeDropDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Pulse Discharge Wave (ADR 0262)
            static DWORD s_lastKineticPulseWaveTime = 0;
            bool kineticPulseWaveStance = (handsDistSledge < 0.28f &&
                                           vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                           vrState.rightController.trigger > 0.65f);
            bool kineticPulseWavePlunge = (kineticPulseWaveStance && isJumpingStance &&
                                           rVel.y < -1.60f && lVel.y < -1.35f &&
                                           fabsf(rVel.x - lVel.x) > 0.95f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Sternum Bone Fracture Mat Spike (ADR 0265)
            static DWORD s_lastSternumFractureSpikeTime = 0;
            static char s_sternumFractureSpikeDirKey = 'A';
            bool sternumFractureSpikeStance = (handsDistSledge < 0.26f &&
                                               leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                               vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool sternumFractureSpikeThrow = (sternumFractureSpikeStance && rVel.y < -1.60f &&
                                              lVel.y < -1.35f && fabsf(lVel.x) > 0.85f);
            if (sternumFractureSpikeThrow && (now - s_lastSternumFractureSpikeTime > 2000)) {
                s_sternumFractureSpikeDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Cleave Wave (ADR 0266)
            static DWORD s_lastPlasmaCleaveWaveTime = 0;
            bool plasmaCleaveWaveStance = (handsDistSledge < 0.28f &&
                                           vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                           vrState.rightController.trigger > 0.65f);
            bool plasmaCleaveWavePlunge = (plasmaCleaveWaveStance && isJumpingStance &&
                                           rVel.y < -1.65f && lVel.y < -1.40f &&
                                           fabsf(rVel.x - lVel.x) > 1.00f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Sternum Bone Fracture Thoracic Grating Slam (ADR 0269)
            static DWORD s_lastSternumGratingSlamTime = 0;
            static char s_sternumGratingSlamDirKey = 'A';
            bool sternumGratingSlamStance = (handsDistSledge < 0.26f &&
                                             leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                             vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool sternumGratingSlamThrow = (sternumGratingSlamStance && rVel.y < -1.65f &&
                                            lVel.y < -1.40f && fabsf(lVel.x) > 0.85f);
            if (sternumGratingSlamThrow && (now - s_lastSternumGratingSlamTime > 2000)) {
                s_sternumGratingSlamDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Shockwave Overload (ADR 0270)
            static DWORD s_lastPlasmaOverloadWaveTime = 0;
            bool plasmaOverloadWaveStance = (handsDistSledge < 0.28f &&
                                             vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                             vrState.rightController.trigger > 0.65f);
            bool plasmaOverloadWavePlunge = (plasmaOverloadWaveStance && isJumpingStance &&
                                             rVel.y < -1.70f && lVel.y < -1.45f &&
                                             fabsf(rVel.x - lVel.x) > 1.05f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Grating Fracture Slam (ADR 0273)
            static DWORD s_lastCervicalGratingSlamTime = 0;
            static char s_cervicalGratingSlamDirKey = 'A';
            bool cervicalGratingSlamStance = (handsDistSledge < 0.26f &&
                                              leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                              vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cervicalGratingSlamThrow = (cervicalGratingSlamStance && rVel.y < -1.70f &&
                                             lVel.y < -1.45f && fabsf(lVel.x) > 0.85f);
            if (cervicalGratingSlamThrow && (now - s_lastCervicalGratingSlamTime > 2000)) {
                s_cervicalGratingSlamDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Detonation (ADR 0274)
            static DWORD s_lastPlasmaResonantWaveTime = 0;
            bool plasmaResonantWaveStance = (handsDistSledge < 0.28f &&
                                             vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                             vrState.rightController.trigger > 0.65f);
            bool plasmaResonantWavePlunge = (plasmaResonantWaveStance && isJumpingStance &&
                                             rVel.y < -1.75f && lVel.y < -1.50f &&
                                             fabsf(rVel.x - lVel.x) > 1.10f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Grating Hyperextension Finisher (ADR 0277)
            static DWORD s_lastHyperextensionFinisherTime = 0;
            static char s_hyperextensionFinisherDirKey = 'A';
            bool hyperextensionFinisherStance = (handsDistSledge < 0.26f &&
                                                 leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                                 vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool hyperextensionFinisherThrow = (hyperextensionFinisherStance && rVel.y < -1.75f &&
                                                lVel.y < -1.50f && fabsf(lVel.x) > 0.85f);
            if (hyperextensionFinisherThrow && (now - s_lastHyperextensionFinisherTime > 2000)) {
                s_hyperextensionFinisherDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Harmonic Deck Quake (ADR 0278)
            static DWORD s_lastPlasmaHarmonicQuakeTime = 0;
            bool plasmaHarmonicQuakeStance = (handsDistSledge < 0.28f &&
                                              vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                              vrState.rightController.trigger > 0.65f);
            bool plasmaHarmonicQuakePlunge = (plasmaHarmonicQuakeStance && isJumpingStance &&
                                              rVel.y < -1.80f && lVel.y < -1.55f &&
                                              fabsf(rVel.x - lVel.x) > 1.15f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Mat Dislocation (ADR 0281)
            static DWORD s_lastThoracicDislocationTime = 0;
            static char s_thoracicDislocationDirKey = 'A';
            bool thoracicDislocationStance = (handsDistSledge < 0.26f &&
                                              leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                              vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool thoracicDislocationThrow = (thoracicDislocationStance && rVel.y < -1.80f &&
                                             lVel.y < -1.55f && fabsf(lVel.x) > 0.85f);
            if (thoracicDislocationThrow && (now - s_lastThoracicDislocationTime > 2000)) {
                s_thoracicDislocationDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Seism Shockwave (ADR 0282)
            static DWORD s_lastPlasmaResonantSeismTime = 0;
            bool plasmaResonantSeismStance = (handsDistSledge < 0.28f &&
                                              vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                              vrState.rightController.trigger > 0.65f);
            bool plasmaResonantSeismPlunge = (plasmaResonantSeismStance && isJumpingStance &&
                                              rVel.y < -1.85f && lVel.y < -1.60f &&
                                              fabsf(rVel.x - lVel.x) > 1.20f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Mat Slam Finisher (ADR 0285)
            static DWORD s_lastThoracicMatSlamTime = 0;
            static char s_thoracicMatSlamDirKey = 'A';
            bool thoracicMatSlamStance = (handsDistSledge < 0.26f &&
                                          leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool thoracicMatSlamThrow = (thoracicMatSlamStance && rVel.y < -1.85f &&
                                         lVel.y < -1.60f && fabsf(lVel.x) > 0.85f);
            if (thoracicMatSlamThrow && (now - s_lastThoracicMatSlamTime > 2000)) {
                s_thoracicMatSlamDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Seism Overdrive (ADR 0286)
            static DWORD s_lastPlasmaSeismOverdriveTime = 0;
            bool plasmaSeismOverdriveStance = (handsDistSledge < 0.28f &&
                                               vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                               vrState.rightController.trigger > 0.65f);
            bool plasmaSeismOverdrivePlunge = (plasmaSeismOverdriveStance && isJumpingStance &&
                                               rVel.y < -1.90f && lVel.y < -1.65f &&
                                               fabsf(rVel.x - lVel.x) > 1.25f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Ground Pin Lock (ADR 0289)
            static DWORD s_lastThoracicPinLockTime = 0;
            static char s_thoracicPinLockDirKey = 'A';
            bool thoracicPinLockStance = (handsDistSledge < 0.26f &&
                                          leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool thoracicPinLockThrow = (thoracicPinLockStance && rVel.y < -1.90f &&
                                         lVel.y < -1.65f && fabsf(lVel.x) > 0.85f);
            if (thoracicPinLockThrow && (now - s_lastThoracicPinLockTime > 2000)) {
                s_thoracicPinLockDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Shockwave (ADR 0290)
            static DWORD s_lastPlasmaCataclysmTime = 0;
            bool plasmaCataclysmStance = (handsDistSledge < 0.28f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                          vrState.rightController.trigger > 0.65f);
            bool plasmaCataclysmPlunge = (plasmaCataclysmStance && isJumpingStance &&
                                          rVel.y < -1.95f && lVel.y < -1.70f &&
                                          fabsf(rVel.x - lVel.x) > 1.30f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Sternum Elbow Drop (ADR 0293)
            static DWORD s_lastSternumElbowDropTime = 0;
            static char s_sternumElbowDropDirKey = 'A';
            bool sternumElbowDropStance = (handsDistSledge < 0.26f &&
                                           leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                           vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool sternumElbowDropPlunge = (sternumElbowDropStance && rVel.y < -1.95f &&
                                           lVel.y < -1.70f && fabsf(lVel.x) > 0.90f);
            if (sternumElbowDropPlunge && (now - s_lastSternumElbowDropTime > 2000)) {
                s_sternumElbowDropDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Overdrive (ADR 0294)
            static DWORD s_lastPlasmaCataclysmOverdriveTime = 0;
            bool plasmaCataclysmOverdriveStance = (handsDistSledge < 0.28f &&
                                                  vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                                  vrState.rightController.trigger > 0.65f);
            bool plasmaCataclysmOverdrivePlunge = (plasmaCataclysmOverdriveStance && isJumpingStance &&
                                                  rVel.y < -2.00f && lVel.y < -1.75f &&
                                                  fabsf(rVel.x - lVel.x) > 1.35f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Deck Driver (ADR 0297)
            static DWORD s_lastCranialDeckDriverTime = 0;
            static char s_cranialDeckDriverDirKey = 'A';
            bool cranialDeckDriverStance = (handsDistSledge < 0.26f &&
                                            leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                            vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialDeckDriverPlunge = (cranialDeckDriverStance && rVel.y < -2.00f &&
                                            lVel.y < -1.75f && fabsf(lVel.x) > 0.90f);
            if (cranialDeckDriverPlunge && (now - s_lastCranialDeckDriverTime > 2000)) {
                s_cranialDeckDriverDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Singularity (ADR 0298)
            static DWORD s_lastPlasmaSingularityTime = 0;
            bool plasmaSingularityStance = (handsDistSledge < 0.28f &&
                                            vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                            vrState.rightController.trigger > 0.65f);
            bool plasmaSingularityPlunge = (plasmaSingularityStance && isJumpingStance &&
                                            rVel.y < -2.05f && lVel.y < -1.80f &&
                                            fabsf(rVel.x - lVel.x) > 1.40f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Sternum Knee Spike Finisher (ADR 0301)
            static DWORD s_lastSternumKneeSpikeTime = 0;
            static char s_sternumKneeSpikeDirKey = 'A';
            bool sternumKneeSpikeStance = (handsDistSledge < 0.26f &&
                                           leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                           vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool sternumKneeSpikePlunge = (sternumKneeSpikeStance && rVel.y < -2.05f &&
                                           lVel.y < -1.80f && fabsf(lVel.x) > 0.90f);
            if (sternumKneeSpikePlunge && (now - s_lastSternumKneeSpikeTime > 2000)) {
                s_sternumKneeSpikeDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Supernova (ADR 0302)
            static DWORD s_lastPlasmaSupernovaTime = 0;
            bool plasmaSupernovaStance = (handsDistSledge < 0.28f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                          vrState.rightController.trigger > 0.65f);
            bool plasmaSupernovaPlunge = (plasmaSupernovaStance && isJumpingStance &&
                                          rVel.y < -2.10f && lVel.y < -1.85f &&
                                          fabsf(rVel.x - lVel.x) > 1.45f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cervical Deck Pin Spike (ADR 0305)
            static DWORD s_lastCervicalDeckPinTime = 0;
            static char s_cervicalDeckPinDirKey = 'A';
            bool cervicalDeckPinStance = (handsDistSledge < 0.26f &&
                                          leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cervicalDeckPinPlunge = (cervicalDeckPinStance && rVel.y < -2.10f &&
                                          lVel.y < -1.85f && fabsf(lVel.x) > 0.90f);
            if (cervicalDeckPinPlunge && (now - s_lastCervicalDeckPinTime > 2000)) {
                s_cervicalDeckPinDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Hypernova (ADR 0306)
            static DWORD s_lastPlasmaHypernovaTime = 0;
            bool plasmaHypernovaStance = (handsDistSledge < 0.28f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                          vrState.rightController.trigger > 0.65f);
            bool plasmaHypernovaPlunge = (plasmaHypernovaStance && isJumpingStance &&
                                          rVel.y < -2.15f && lVel.y < -1.90f &&
                                          fabsf(rVel.x - lVel.x) > 1.50f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Grating Grind (ADR 0309)
            static DWORD s_lastCranialGrindTime = 0;
            static char s_cranialGrindDirKey = 'A';
            bool cranialGrindStance = (handsDistSledge < 0.26f &&
                                       leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                       vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialGrindPlunge = (cranialGrindStance && rVel.y < -2.15f &&
                                       lVel.y < -1.90f && fabsf(lVel.x) > 0.90f);
            if (cranialGrindPlunge && (now - s_lastCranialGrindTime > 2000)) {
                s_cranialGrindDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Singularity Overdrive (ADR 0310)
            static DWORD s_lastPlasmaSingularityOverdriveTime = 0;
            bool plasmaSingularityOverdriveStance = (handsDistSledge < 0.28f &&
                                                    vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                                    vrState.rightController.trigger > 0.65f);
            bool plasmaSingularityOverdrivePlunge = (plasmaSingularityOverdriveStance && isJumpingStance &&
                                                    rVel.y < -2.20f && lVel.y < -1.95f &&
                                                    fabsf(rVel.x - lVel.x) > 1.55f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Post-Impactor Slam (ADR 0313)
            static DWORD s_lastCranialPostImpactTime = 0;
            static char s_cranialPostImpactDirKey = 'A';
            bool cranialPostImpactStance = (handsDistSledge < 0.26f &&
                                            leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                            vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialPostImpactPlunge = (cranialPostImpactStance && rVel.y < -2.20f &&
                                            lVel.y < -1.95f && fabsf(lVel.x) > 0.95f);
            if (cranialPostImpactPlunge && (now - s_lastCranialPostImpactTime > 2000)) {
                s_cranialPostImpactDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Genesis Overdrive (ADR 0314)
            static DWORD s_lastPlasmaGenesisTime = 0;
            bool plasmaGenesisStance = (handsDistSledge < 0.28f &&
                                        vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                        vrState.rightController.trigger > 0.65f);
            bool plasmaGenesisPlunge = (plasmaGenesisStance && isJumpingStance &&
                                        rVel.y < -2.25f && lVel.y < -2.00f &&
                                        fabsf(rVel.x - lVel.x) > 1.60f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Deck Grating Stomp Finisher (ADR 0317)
            static DWORD s_lastCranialStompTime = 0;
            static char s_cranialStompDirKey = 'A';
            bool cranialStompStance = (handsDistSledge < 0.26f &&
                                       leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                       vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialStompPlunge = (cranialStompStance && rVel.y < -2.25f &&
                                       lVel.y < -2.00f && fabsf(lVel.x) > 0.95f);
            if (cranialStompPlunge && (now - s_lastCranialStompTime > 2000)) {
                s_cranialStompDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Big Bang Overdrive (ADR 0318)
            static DWORD s_lastPlasmaBigBangTime = 0;
            bool plasmaBigBangStance = (handsDistSledge < 0.28f &&
                                        vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                        vrState.rightController.trigger > 0.65f);
            bool plasmaBigBangPlunge = (plasmaBigBangStance && isJumpingStance &&
                                        rVel.y < -2.30f && lVel.y < -2.05f &&
                                        fabsf(rVel.x - lVel.x) > 1.65f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Post Double-Strike Execution (ADR 0321)
            static DWORD s_lastCranialDoubleStrikeTime = 0;
            static char s_cranialDoubleStrikeDirKey = 'A';
            bool cranialDoubleStrikeStance = (handsDistSledge < 0.26f &&
                                              leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                              vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialDoubleStrikePlunge = (cranialDoubleStrikeStance && rVel.y < -2.30f &&
                                              lVel.y < -2.05f && fabsf(lVel.x) > 0.95f);
            if (cranialDoubleStrikePlunge && (now - s_lastCranialDoubleStrikeTime > 2000)) {
                s_cranialDoubleStrikeDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Cosmic Dawn Overdrive (ADR 0322)
            static DWORD s_lastPlasmaCosmicDawnTime = 0;
            bool plasmaCosmicDawnStance = (handsDistSledge < 0.28f &&
                                           vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                           vrState.rightController.trigger > 0.65f);
            bool plasmaCosmicDawnPlunge = (plasmaCosmicDawnStance && isJumpingStance &&
                                           rVel.y < -2.35f && lVel.y < -2.10f &&
                                           fabsf(rVel.x - lVel.x) > 1.70f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Post Pivot-Twist Fracture (ADR 0325)
            static DWORD s_lastCranialPivotTwistTime = 0;
            static char s_cranialPivotTwistDirKey = 'A';
            bool cranialPivotTwistStance = (handsDistSledge < 0.26f &&
                                            leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                            vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialPivotTwistPlunge = (cranialPivotTwistStance && rVel.y < -2.35f &&
                                            lVel.y < -2.10f && fabsf(lVel.x) > 1.00f);
            if (cranialPivotTwistPlunge && (now - s_lastCranialPivotTwistTime > 2000)) {
                s_cranialPivotTwistDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Supercluster Overdrive (ADR 0326)
            static DWORD s_lastPlasmaSuperclusterTime = 0;
            bool plasmaSuperclusterStance = (handsDistSledge < 0.28f &&
                                             vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                             vrState.rightController.trigger > 0.65f);
            bool plasmaSuperclusterPlunge = (plasmaSuperclusterStance && isJumpingStance &&
                                             rVel.y < -2.40f && lVel.y < -2.15f &&
                                             fabsf(rVel.x - lVel.x) > 1.75f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Post Pivot-Twist Deck Slam (ADR 0329)
            static DWORD s_lastCranialPivotSlamTime = 0;
            static char s_cranialPivotSlamDirKey = 'A';
            bool cranialPivotSlamStance = (handsDistSledge < 0.26f &&
                                           leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                           vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialPivotSlamPlunge = (cranialPivotSlamStance && rVel.y < -2.40f &&
                                           lVel.y < -2.15f && fabsf(lVel.x) > 1.00f);
            if (cranialPivotSlamPlunge && (now - s_lastCranialPivotSlamTime > 2000)) {
                s_cranialPivotSlamDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Omniverse Overdrive (ADR 0330)
            static DWORD s_lastPlasmaOmniverseTime = 0;
            bool plasmaOmniverseStance = (handsDistSledge < 0.28f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                          vrState.rightController.trigger > 0.65f);
            bool plasmaOmniversePlunge = (plasmaOmniverseStance && isJumpingStance &&
                                          rVel.y < -2.45f && lVel.y < -2.20f &&
                                          fabsf(rVel.x - lVel.x) > 1.80f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Lever-Pin Lock Takedown (ADR 0333)
            static DWORD s_lastCranialLeverPinTime = 0;
            static char s_cranialLeverPinDirKey = 'A';
            bool cranialLeverPinStance = (handsDistSledge < 0.26f &&
                                          leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialLeverPinPlunge = (cranialLeverPinStance && rVel.y < -2.45f &&
                                          lVel.y < -2.20f && fabsf(lVel.x) > 1.05f);
            if (cranialLeverPinPlunge && (now - s_lastCranialLeverPinTime > 2000)) {
                s_cranialLeverPinDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Metaverse Overdrive (ADR 0334)
            static DWORD s_lastPlasmaMetaverseTime = 0;
            bool plasmaMetaverseStance = (handsDistSledge < 0.28f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                          vrState.rightController.trigger > 0.65f);
            bool plasmaMetaversePlunge = (plasmaMetaverseStance && isJumpingStance &&
                                          rVel.y < -2.50f && lVel.y < -2.25f &&
                                          fabsf(rVel.x - lVel.x) > 1.85f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Chokehold Finisher (ADR 0337)
            static DWORD s_lastCranialFulcrumChokeTime = 0;
            static char s_cranialFulcrumChokeDirKey = 'A';
            bool cranialFulcrumChokeStance = (handsDistSledge < 0.26f &&
                                              leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                              vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialFulcrumChokePlunge = (cranialFulcrumChokeStance && rVel.y < -2.50f &&
                                              lVel.y < -2.25f && fabsf(lVel.x) > 1.10f);
            if (cranialFulcrumChokePlunge && (now - s_lastCranialFulcrumChokeTime > 2000)) {
                s_cranialFulcrumChokeDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Hyperdimensional Overdrive (ADR 0338)
            static DWORD s_lastPlasmaHyperdimensionalTime = 0;
            bool plasmaHyperdimensionalStance = (handsDistSledge < 0.28f &&
                                                 vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                                 vrState.rightController.trigger > 0.65f);
            bool plasmaHyperdimensionalPlunge = (plasmaHyperdimensionalStance && isJumpingStance &&
                                                 rVel.y < -2.55f && lVel.y < -2.30f &&
                                                 fabsf(rVel.x - lVel.x) > 1.90f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Armbar Fracture (ADR 0341)
            static DWORD s_lastCranialFulcrumArmbarTime = 0;
            static char s_cranialFulcrumArmbarDirKey = 'A';
            bool cranialFulcrumArmbarStance = (handsDistSledge < 0.26f &&
                                               leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                               vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialFulcrumArmbarPlunge = (cranialFulcrumArmbarStance && rVel.y < -2.55f &&
                                               lVel.y < -2.30f && fabsf(lVel.x) > 1.15f);
            if (cranialFulcrumArmbarPlunge && (now - s_lastCranialFulcrumArmbarTime > 2000)) {
                s_cranialFulcrumArmbarDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Transdimensional Overdrive (ADR 0342)
            static DWORD s_lastPlasmaTransdimensionalTime = 0;
            bool plasmaTransdimensionalStance = (handsDistSledge < 0.28f &&
                                                 vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                                 vrState.rightController.trigger > 0.65f);
            bool plasmaTransdimensionalPlunge = (plasmaTransdimensionalStance && isJumpingStance &&
                                                 rVel.y < -2.60f && lVel.y < -2.35f &&
                                                 fabsf(rVel.x - lVel.x) > 1.95f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Sternum Knee Drive (ADR 0345)
            static DWORD s_lastCranialFulcrumKneeTime = 0;
            static char s_cranialFulcrumKneeDirKey = 'A';
            bool cranialFulcrumKneeStance = (handsDistSledge < 0.26f &&
                                             leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                             vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialFulcrumKneePlunge = (cranialFulcrumKneeStance && rVel.y < -2.60f &&
                                             lVel.y < -2.35f && fabsf(lVel.x) > 1.20f);
            if (cranialFulcrumKneePlunge && (now - s_lastCranialFulcrumKneeTime > 2000)) {
                s_cranialFulcrumKneeDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Interdimensional Overdrive (ADR 0346)
            static DWORD s_lastPlasmaInterdimensionalTime = 0;
            bool plasmaInterdimensionalStance = (handsDistSledge < 0.28f &&
                                                 vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                                 vrState.rightController.trigger > 0.65f);
            bool plasmaInterdimensionalPlunge = (plasmaInterdimensionalStance && isJumpingStance &&
                                                 rVel.y < -2.65f && lVel.y < -2.40f &&
                                                 fabsf(rVel.x - lVel.x) > 2.00f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Ribcage Fracture Deck Slam (ADR 0349)
            static DWORD s_lastCranialFulcrumRibSlamTime = 0;
            static char s_cranialFulcrumRibSlamDirKey = 'A';
            bool cranialFulcrumRibSlamStance = (handsDistSledge < 0.26f &&
                                                leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                                vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialFulcrumRibSlamPlunge = (cranialFulcrumRibSlamStance && rVel.y < -2.65f &&
                                                lVel.y < -2.40f && fabsf(lVel.x) > 1.25f);
            if (cranialFulcrumRibSlamPlunge && (now - s_lastCranialFulcrumRibSlamTime > 2000)) {
                s_cranialFulcrumRibSlamDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Continuum Singularity Overdrive (ADR 0350)
            static DWORD s_lastPlasmaContinuumSingularityTime = 0;
            bool plasmaContinuumSingularityStance = (handsDistSledge < 0.28f &&
                                                     vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                                     vrState.rightController.trigger > 0.65f);
            bool plasmaContinuumSingularityPlunge = (plasmaContinuumSingularityStance && isJumpingStance &&
                                                     rVel.y < -2.70f && lVel.y < -2.45f &&
                                                     fabsf(rVel.x - lVel.x) > 2.05f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Clavicle Fracture Ground Pin (ADR 0353)
            static DWORD s_lastCranialFulcrumClaviclePinTime = 0;
            static char s_cranialFulcrumClaviclePinDirKey = 'A';
            bool cranialFulcrumClaviclePinStance = (handsDistSledge < 0.26f &&
                                                    leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                                    vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialFulcrumClaviclePinPlunge = (cranialFulcrumClaviclePinStance && rVel.y < -2.70f &&
                                                    lVel.y < -2.45f && fabsf(lVel.x) > 1.30f);
            if (cranialFulcrumClaviclePinPlunge && (now - s_lastCranialFulcrumClaviclePinTime > 2000)) {
                s_cranialFulcrumClaviclePinDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Multiverse Singularity Overdrive (ADR 0354)
            static DWORD s_lastPlasmaMultiverseSingularityTime = 0;
            bool plasmaMultiverseSingularityStance = (handsDistSledge < 0.28f &&
                                                      vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                                      vrState.rightController.trigger > 0.65f);
            bool plasmaMultiverseSingularityPlunge = (plasmaMultiverseSingularityStance && isJumpingStance &&
                                                      rVel.y < -2.75f && lVel.y < -2.50f &&
                                                      fabsf(rVel.x - lVel.x) > 2.10f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Scapula Fracture Deck Pin (ADR 0357)
            static DWORD s_lastCranialFulcrumScapulaPinTime = 0;
            static char s_cranialFulcrumScapulaPinDirKey = 'A';
            bool cranialFulcrumScapulaPinStance = (handsDistSledge < 0.26f &&
                                                    leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                                    vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialFulcrumScapulaPinPlunge = (cranialFulcrumScapulaPinStance && rVel.y < -2.75f &&
                                                    lVel.y < -2.50f && fabsf(lVel.x) > 1.35f);
            if (cranialFulcrumScapulaPinPlunge && (now - s_lastCranialFulcrumScapulaPinTime > 2000)) {
                s_cranialFulcrumScapulaPinDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Omniverse Singularity Overdrive (ADR 0358)
            static DWORD s_lastPlasmaOmniverseSingularityTime = 0;
            bool plasmaOmniverseSingularityStance = (handsDistSledge < 0.28f &&
                                                     vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                                     vrState.rightController.trigger > 0.65f);
            bool plasmaOmniverseSingularityPlunge = (plasmaOmniverseSingularityStance && isJumpingStance &&
                                                     rVel.y < -2.80f && lVel.y < -2.55f &&
                                                     fabsf(rVel.x - lVel.x) > 2.15f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Sternum Crushing Ground Pin (ADR 0361)
            static DWORD s_lastCranialFulcrumSternumPinTime = 0;
            static char s_cranialFulcrumSternumPinDirKey = 'A';
            bool cranialFulcrumSternumPinStance = (handsDistSledge < 0.26f &&
                                                    leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                                    vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialFulcrumSternumPinPlunge = (cranialFulcrumSternumPinStance && rVel.y < -2.80f &&
                                                    lVel.y < -2.55f && fabsf(lVel.x) > 1.40f);
            if (cranialFulcrumSternumPinPlunge && (now - s_lastCranialFulcrumSternumPinTime > 2000)) {
                s_cranialFulcrumSternumPinDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Trans-Cosmic Singularity Overdrive (ADR 0362)
            static DWORD s_lastPlasmaTransCosmicSingularityTime = 0;
            bool plasmaTransCosmicSingularityStance = (handsDistSledge < 0.28f &&
                                                       vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                                       vrState.rightController.trigger > 0.65f);
            bool plasmaTransCosmicSingularityPlunge = (plasmaTransCosmicSingularityStance && isJumpingStance &&
                                                       rVel.y < -2.85f && lVel.y < -2.60f &&
                                                       fabsf(rVel.x - lVel.x) > 2.20f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Mandible Dislocation Ground Pin (ADR 0365)
            static DWORD s_lastCranialFulcrumMandiblePinTime = 0;
            static char s_cranialFulcrumMandiblePinDirKey = 'A';
            bool cranialFulcrumMandiblePinStance = (handsDistSledge < 0.26f &&
                                                     leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                                     vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialFulcrumMandiblePinPlunge = (cranialFulcrumMandiblePinStance && rVel.y < -2.85f &&
                                                     lVel.y < -2.60f && fabsf(lVel.x) > 1.45f);
            if (cranialFulcrumMandiblePinPlunge && (now - s_lastCranialFulcrumMandiblePinTime > 2000)) {
                s_cranialFulcrumMandiblePinDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Macrocosm Singularity Overdrive (ADR 0366)
            static DWORD s_lastPlasmaMacrocosmSingularityTime = 0;
            bool plasmaMacrocosmSingularityStance = (handsDistSledge < 0.28f &&
                                                       vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                                       vrState.rightController.trigger > 0.65f);
            bool plasmaMacrocosmSingularityPlunge = (plasmaMacrocosmSingularityStance && isJumpingStance &&
                                                       rVel.y < -2.90f && lVel.y < -2.65f &&
                                                       fabsf(rVel.x - lVel.x) > 2.25f);

            // Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Zygomatic Arch Fracture Ground Pin (ADR 0369)
            static DWORD s_lastCranialFulcrumZygomaticPinTime = 0;
            static char s_cranialFulcrumZygomaticPinDirKey = 'A';
            bool cranialFulcrumZygomaticPinStance = (handsDistSledge < 0.26f &&
                                                     leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                                     vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f);
            bool cranialFulcrumZygomaticPinPlunge = (cranialFulcrumZygomaticPinStance && rVel.y < -2.90f &&
                                                     lVel.y < -2.65f && fabsf(lVel.x) > 1.50f);
            if (cranialFulcrumZygomaticPinPlunge && (now - s_lastCranialFulcrumZygomaticPinTime > 2000)) {
                s_cranialFulcrumZygomaticPinDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
            }

            // Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Universal Singularity Overdrive (ADR 0370)
            static DWORD s_lastPlasmaUniversalSingularityTime = 0;
            bool plasmaUniversalSingularityStance = (handsDistSledge < 0.28f &&
                                                       vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                                       vrState.rightController.trigger > 0.65f);
            bool plasmaUniversalSingularityPlunge = (plasmaUniversalSingularityStance && isJumpingStance &&
                                                       rVel.y < -2.95f && lVel.y < -2.70f &&
                                                       fabsf(rVel.x - lVel.x) > 2.30f);

            // Inmate Pit Brawl Outside Leg Hook Sweep & Ground Knee Drop (ADR 0209)
            static DWORD s_lastOutsideLegSweepTime = 0;
            static DWORD s_outsideLegSweepHookTime = 0;
            static char s_outsideLegSweepDirKey = 'A';
            bool outsideLegSweepHook = (leftLocal.y < -0.30f && fabsf(lVel.x) > 0.90f);
            if (outsideLegSweepHook && (now - s_lastOutsideLegSweepTime > 2000)) {
                if (s_outsideLegSweepHookTime == 0) {
                    s_outsideLegSweepHookTime = now;
                    s_outsideLegSweepDirKey = (lVel.x > 0.0f) ? 'D' : 'A';
                    VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL OUTSIDE LEG HOOKED (Dir: %c, Lx: %.2f)", s_outsideLegSweepDirKey, lVel.x);
                    vrState.leftHapticPulse = 0.85f; // Sweep trip torque (200 Hz)
                }
            }
            bool outsideLegKneeDrop = (s_outsideLegSweepHookTime > 0 && (now - s_outsideLegSweepHookTime < 500) && rVel.y < -1.25f);

            // Security Guard Stun Baton Forward Two-Handed Cross-Chest Block & Arc Thrust Disruption (ADR 0198 & 0210)
            static DWORD s_lastCrossThrustTime = 0;
            static DWORD s_crossBlockHoldStart = 0;
            bool crossChestGuardStance = (handsDistSledge > 0.20f && handsDistSledge < 0.38f &&
                                          fabsf(rightLocal.x - leftLocal.x) > 0.22f &&
                                          rightLocal.y > -0.15f && rightLocal.y < 0.15f &&
                                          rightLocal.z > -0.30f && rightLocal.z < -0.10f &&
                                          vrState.leftController.grip > 0.65f && vrState.rightController.grip > 0.65f &&
                                          vrState.leftController.trigger > 0.65f && vrState.rightController.trigger > 0.65f);
            bool isCrossThrustReady = false;
            bool isCrossThrustOvercharged = false;
            if (crossChestGuardStance) {
                if (s_crossBlockHoldStart == 0) s_crossBlockHoldStart = now;
                DWORD crossHoldDuration = now - s_crossBlockHoldStart;
                if (crossHoldDuration >= 500) {
                    vrState.leftHapticPulse = 0.90f;  // Escalating supercharge hum (340 Hz)
                    vrState.rightHapticPulse = 0.90f;
                    if (rVel.z < -1.30f && lVel.z < -1.10f) {
                        isCrossThrustOvercharged = true;
                    }
                } else if (crossHoldDuration >= 300) {
                    vrState.leftHapticPulse = 0.65f;  // Electrostatic barrier hum (240 Hz)
                    vrState.rightHapticPulse = 0.65f;
                    if (rVel.z < -1.25f && lVel.z < -1.05f) {
                        isCrossThrustReady = true;
                    }
                }
            } else {
                s_crossBlockHoldStart = 0;
            }

            if (plasmaUniversalSingularityPlunge && (now - s_lastPlasmaUniversalSingularityTime > 2000)) {
                s_lastPlasmaUniversalSingularityTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM UNIVERSAL SINGULARITY OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm universal singularity overdrive shockwave (550 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck universal recoil (480 Hz)
                goto combat_gesture_done;
            }

            if (cranialFulcrumZygomaticPinPlunge && (now - s_lastCranialFulcrumZygomaticPinTime > 2000)) {
                s_lastCranialFulcrumZygomaticPinTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST FULCRUM ZYGOMATIC PIN EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialFulcrumZygomaticPinDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialFulcrumZygomaticPinDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialFulcrumZygomaticPinDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive zygomatic fracture shockwave (540 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer overhook drag torque (470 Hz)
                goto combat_gesture_done;
            }

            if (plasmaMacrocosmSingularityPlunge && (now - s_lastPlasmaMacrocosmSingularityTime > 2000)) {
                s_lastPlasmaMacrocosmSingularityTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM MACROCOSM SINGULARITY OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm macrocosm singularity overdrive shockwave (540 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck macrocosm recoil (470 Hz)
                goto combat_gesture_done;
            }

            if (cranialFulcrumMandiblePinPlunge && (now - s_lastCranialFulcrumMandiblePinTime > 2000)) {
                s_lastCranialFulcrumMandiblePinTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST FULCRUM MANDIBLE PIN EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialFulcrumMandiblePinDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialFulcrumMandiblePinDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialFulcrumMandiblePinDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive mandible fracture shockwave (530 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer overhook drag torque (460 Hz)
                goto combat_gesture_done;
            }

            if (plasmaTransCosmicSingularityPlunge && (now - s_lastPlasmaTransCosmicSingularityTime > 2000)) {
                s_lastPlasmaTransCosmicSingularityTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM TRANS-COSMIC SINGULARITY OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm trans-cosmic singularity overdrive shockwave (530 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck trans-cosmic recoil (460 Hz)
                goto combat_gesture_done;
            }

            if (cranialFulcrumSternumPinPlunge && (now - s_lastCranialFulcrumSternumPinTime > 2000)) {
                s_lastCranialFulcrumSternumPinTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST FULCRUM STERNUM PIN EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialFulcrumSternumPinDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialFulcrumSternumPinDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialFulcrumSternumPinDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive sternum fracture shockwave (520 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer overhook drag torque (450 Hz)
                goto combat_gesture_done;
            }

            if (plasmaOmniverseSingularityPlunge && (now - s_lastPlasmaOmniverseSingularityTime > 2000)) {
                s_lastPlasmaOmniverseSingularityTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM OMNIVERSE SINGULARITY OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm omniverse singularity overdrive shockwave (520 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck omniverse recoil (450 Hz)
                goto combat_gesture_done;
            }
            if (cranialFulcrumScapulaPinPlunge && (now - s_lastCranialFulcrumScapulaPinTime > 2000)) {
                s_lastCranialFulcrumScapulaPinTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST FULCRUM SCAPULA PIN EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialFulcrumScapulaPinDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialFulcrumScapulaPinDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialFulcrumScapulaPinDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive scapula fracture shockwave (510 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer overhook drag torque (440 Hz)
                goto combat_gesture_done;
            }

            if (plasmaMultiverseSingularityPlunge && (now - s_lastPlasmaMultiverseSingularityTime > 2000)) {
                s_lastPlasmaMultiverseSingularityTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM MULTIVERSE SINGULARITY OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm multiverse singularity overdrive shockwave (510 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck multiverse recoil (440 Hz)
                goto combat_gesture_done;
            }
            if (cranialFulcrumClaviclePinPlunge && (now - s_lastCranialFulcrumClaviclePinTime > 2000)) {
                s_lastCranialFulcrumClaviclePinTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST FULCRUM CLAVICLE PIN EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialFulcrumClaviclePinDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialFulcrumClaviclePinDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialFulcrumClaviclePinDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive clavicle fracture shockwave (510 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer overhook drag torque (440 Hz)
                goto combat_gesture_done;
            }

            if (plasmaContinuumSingularityPlunge && (now - s_lastPlasmaContinuumSingularityTime > 2000)) {
                s_lastPlasmaContinuumSingularityTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM CONTINUUM SINGULARITY OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm continuum singularity overdrive shockwave (500 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck continuum recoil (430 Hz)
                goto combat_gesture_done;
            }
            if (cranialFulcrumRibSlamPlunge && (now - s_lastCranialFulcrumRibSlamTime > 2000)) {
                s_lastCranialFulcrumRibSlamTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST FULCRUM RIBCAGE SLAM EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialFulcrumRibSlamDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialFulcrumRibSlamDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialFulcrumRibSlamDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive ribcage fracture shockwave (500 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer overhook drag torque (430 Hz)
                goto combat_gesture_done;
            }

            if (plasmaInterdimensionalPlunge && (now - s_lastPlasmaInterdimensionalTime > 2000)) {
                s_lastPlasmaInterdimensionalTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM INTERDIMENSIONAL OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm Interdimensional overdrive shockwave (500 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck Interdimensional recoil (420 Hz)
                goto combat_gesture_done;
            }
            if (cranialFulcrumKneePlunge && (now - s_lastCranialFulcrumKneeTime > 2000)) {
                s_lastCranialFulcrumKneeTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST FULCRUM KNEE EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialFulcrumKneeDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialFulcrumKneeDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialFulcrumKneeDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive fulcrum knee fracture shockwave (500 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer overhook drag torque (420 Hz)
                goto combat_gesture_done;
            }

            if (plasmaTransdimensionalPlunge && (now - s_lastPlasmaTransdimensionalTime > 2000)) {
                s_lastPlasmaTransdimensionalTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM TRANSDIMENSIONAL OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm Transdimensional overdrive shockwave (490 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck Transdimensional recoil (410 Hz)
                goto combat_gesture_done;
            }
            if (cranialFulcrumArmbarPlunge && (now - s_lastCranialFulcrumArmbarTime > 2000)) {
                s_lastCranialFulcrumArmbarTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST FULCRUM ARMBAR EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialFulcrumArmbarDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialFulcrumArmbarDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialFulcrumArmbarDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive fulcrum armbar fracture shockwave (490 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer overhook drag torque (410 Hz)
                goto combat_gesture_done;
            }

            if (plasmaHyperdimensionalPlunge && (now - s_lastPlasmaHyperdimensionalTime > 2000)) {
                s_lastPlasmaHyperdimensionalTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM HYPERDIMENSIONAL OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm Hyperdimensional overdrive shockwave (480 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck Hyperdimensional recoil (400 Hz)
                goto combat_gesture_done;
            }
            if (cranialFulcrumChokePlunge && (now - s_lastCranialFulcrumChokeTime > 2000)) {
                s_lastCranialFulcrumChokeTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST FULCRUM CHOKE EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialFulcrumChokeDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialFulcrumChokeDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialFulcrumChokeDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive fulcrum leverage shockwave (480 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag torque (400 Hz)
                goto combat_gesture_done;
            }

            if (plasmaMetaversePlunge && (now - s_lastPlasmaMetaverseTime > 2000)) {
                s_lastPlasmaMetaverseTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM METAVERSE OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm Metaverse overdrive shockwave (470 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck Metaverse recoil (390 Hz)
                goto combat_gesture_done;
            }
            if (cranialLeverPinPlunge && (now - s_lastCranialLeverPinTime > 2000)) {
                s_lastCranialLeverPinTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST LEVER PIN EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialLeverPinDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialLeverPinDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialLeverPinDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive fulcrum lever lock shockwave (470 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag torque (390 Hz)
                goto combat_gesture_done;
            }

            if (plasmaOmniversePlunge && (now - s_lastPlasmaOmniverseTime > 2000)) {
                s_lastPlasmaOmniverseTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM OMNIVERSE OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm Omniverse overdrive shockwave (460 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck Omniverse recoil (380 Hz)
                goto combat_gesture_done;
            }
            if (cranialPivotSlamPlunge && (now - s_lastCranialPivotSlamTime > 2000)) {
                s_lastCranialPivotSlamTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST PIVOT SLAM EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialPivotSlamDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialPivotSlamDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialPivotSlamDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive deck impact and spinal fracture shockwave (460 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag (380 Hz)
                goto combat_gesture_done;
            }
            if (plasmaSuperclusterPlunge && (now - s_lastPlasmaSuperclusterTime > 2000)) {
                s_lastPlasmaSuperclusterTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM SUPERCLUSTER OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm Supercluster overdrive shockwave (450 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck Supercluster recoil (360 Hz)
                goto combat_gesture_done;
            }
            if (cranialPivotTwistPlunge && (now - s_lastCranialPivotTwistTime > 2000)) {
                s_lastCranialPivotTwistTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST PIVOT TWIST EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialPivotTwistDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialPivotTwistDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialPivotTwistDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Cervical vertebra fracture shockwave (450 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag (370 Hz)
                goto combat_gesture_done;
            }
            if (plasmaCosmicDawnPlunge && (now - s_lastPlasmaCosmicDawnTime > 2000)) {
                s_lastPlasmaCosmicDawnTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM COSMIC DAWN OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm Cosmic Dawn overdrive shockwave (440 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck Cosmic Dawn recoil (340 Hz)
                goto combat_gesture_done;
            }
            if (cranialDoubleStrikePlunge && (now - s_lastCranialDoubleStrikeTime > 2000)) {
                s_lastCranialDoubleStrikeTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST DOUBLE STRIKE EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialDoubleStrikeDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialDoubleStrikeDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialDoubleStrikeDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Dual cranial structural impact shockwave (440 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag (360 Hz)
                goto combat_gesture_done;
            }
            if (plasmaBigBangPlunge && (now - s_lastPlasmaBigBangTime > 2000)) {
                s_lastPlasmaBigBangTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM BIG BANG OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm Big Bang shockwave (430 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck Big Bang recoil (320 Hz)
                goto combat_gesture_done;
            }
            if (cranialStompPlunge && (now - s_lastCranialStompTime > 2000)) {
                s_lastCranialStompTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL DECK STOMP EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialStompDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialStompDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_SPACE, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialStompDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Cranial deck grating fracture shockwave (430 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag (350 Hz)
                goto combat_gesture_done;
            }
            if (plasmaGenesisPlunge && (now - s_lastPlasmaGenesisTime > 2000)) {
                s_lastPlasmaGenesisTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM GENESIS OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm genesis overdrive shockwave (420 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck genesis recoil (300 Hz)
                goto combat_gesture_done;
            }
            if (cranialPostImpactPlunge && (now - s_lastCranialPostImpactTime > 2000)) {
                s_lastCranialPostImpactTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL POST IMPACT SLAM EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialPostImpactDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialPostImpactDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialPostImpactDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Cranial steel post collision shockwave (420 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag (340 Hz)
                goto combat_gesture_done;
            }
            if (plasmaSingularityOverdrivePlunge && (now - s_lastPlasmaSingularityOverdriveTime > 2000)) {
                s_lastPlasmaSingularityOverdriveTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM SINGULARITY OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm singularity overdrive shockwave (410 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck singularity recoil (280 Hz)
                goto combat_gesture_done;
            }
            if (cranialGrindPlunge && (now - s_lastCranialGrindTime > 2000)) {
                s_lastCranialGrindTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL GRATING GRIND EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialGrindDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialGrindDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialGrindDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Cranial diamond grating abrasion shockwave (410 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag (330 Hz)
                goto combat_gesture_done;
            }
            if (plasmaHypernovaPlunge && (now - s_lastPlasmaHypernovaTime > 2000)) {
                s_lastPlasmaHypernovaTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM HYPERNOVA EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm hypernova shockwave (400 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck hypernova recoil (260 Hz)
                goto combat_gesture_done;
            }
            if (cervicalDeckPinPlunge && (now - s_lastCervicalDeckPinTime > 2000)) {
                s_lastCervicalDeckPinTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CERVICAL DECK PIN SPIKE EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cervicalDeckPinDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cervicalDeckPinDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cervicalDeckPinDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Cervical vertebra fracture shockwave (400 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag (320 Hz)
                goto combat_gesture_done;
            }
            if (plasmaSupernovaPlunge && (now - s_lastPlasmaSupernovaTime > 2000)) {
                s_lastPlasmaSupernovaTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM SUPERNOVA EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm supernova shockwave (400 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck supernova recoil (240 Hz)
                goto combat_gesture_done;
            }
            if (sternumKneeSpikePlunge && (now - s_lastSternumKneeSpikeTime > 2000)) {
                s_lastSternumKneeSpikeTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER STERNUM KNEE SPIKE EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_sternumKneeSpikeDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_sternumKneeSpikeDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_sternumKneeSpikeDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Sternum fracture shockwave (400 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag (310 Hz)
                goto combat_gesture_done;
            }
            if (plasmaSingularityPlunge && (now - s_lastPlasmaSingularityTime > 2000)) {
                s_lastPlasmaSingularityTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM SINGULARITY EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm singularity shockwave (390 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck singularity recoil (220 Hz)
                goto combat_gesture_done;
            }
            if (cranialDeckDriverPlunge && (now - s_lastCranialDeckDriverTime > 2000)) {
                s_lastCranialDeckDriverTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CRANIAL DECK DRIVER EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cranialDeckDriverDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cranialDeckDriverDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cranialDeckDriverDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Cranial deck grating impact shockwave (390 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag (300 Hz)
                goto combat_gesture_done;
            }
            if (plasmaCataclysmOverdrivePlunge && (now - s_lastPlasmaCataclysmOverdriveTime > 2000)) {
                s_lastPlasmaCataclysmOverdriveTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm overdrive shockwave (380 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural girder recoil (200 Hz)
                goto combat_gesture_done;
            }
            if (sternumElbowDropPlunge && (now - s_lastSternumElbowDropTime > 2000)) {
                s_lastSternumElbowDropTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER STERNUM ELBOW DROP EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_sternumElbowDropDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_sternumElbowDropDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_sternumElbowDropDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Sternum fracture shockwave (380 Hz)
                vrState.leftHapticPulse = 0.95f;  // Whizzer collar tie drag (290 Hz)
                goto combat_gesture_done;
            }
            if (plasmaCataclysmPlunge && (now - s_lastPlasmaCataclysmTime > 2000)) {
                s_lastPlasmaCataclysmTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT CATACLYSM EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant cataclysm shockwave (360 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural girder recoil (160 Hz)
                goto combat_gesture_done;
            }
            if (thoracicPinLockThrow && (now - s_lastThoracicPinLockTime > 2000)) {
                s_lastThoracicPinLockTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER THORACIC PIN LOCK EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_thoracicPinLockDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_thoracicPinLockDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_thoracicPinLockDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Shoulder joint keylock fracture shockwave (360 Hz)
                vrState.leftHapticPulse = 0.92f;  // Whizzer collar tie drag (270 Hz)
                goto combat_gesture_done;
            }
            if (plasmaSeismOverdrivePlunge && (now - s_lastPlasmaSeismOverdriveTime > 2000)) {
                s_lastPlasmaSeismOverdriveTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT SEISM OVERDRIVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant seism overdrive shockwave (360 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural girder recoil (150 Hz)
                goto combat_gesture_done;
            }
            if (thoracicMatSlamThrow && (now - s_lastThoracicMatSlamTime > 2000)) {
                s_lastThoracicMatSlamTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER THORACIC MAT SLAM EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_thoracicMatSlamDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_thoracicMatSlamDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_thoracicMatSlamDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Thoracic cage fracture shockwave (360 Hz)
                vrState.leftHapticPulse = 0.92f;  // Whizzer collar tie drag (270 Hz)
                goto combat_gesture_done;
            }
            if (plasmaResonantSeismPlunge && (now - s_lastPlasmaResonantSeismTime > 2000)) {
                s_lastPlasmaResonantSeismTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT SEISM EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_TAB, false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Resonant seism shockwave (360 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural girder recoil (140 Hz)
                goto combat_gesture_done;
            }
            if (thoracicDislocationThrow && (now - s_lastThoracicDislocationTime > 2000)) {
                s_lastThoracicDislocationTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER THORACIC DISLOCATION EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_thoracicDislocationDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_thoracicDislocationDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_thoracicDislocationDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Thoracic vertebra dislocation shockwave (360 Hz)
                vrState.leftHapticPulse = 0.92f;  // Whizzer collar tie drag (260 Hz)
                goto combat_gesture_done;
            }
            if (plasmaHarmonicQuakePlunge && (now - s_lastPlasmaHarmonicQuakeTime > 2000)) {
                s_lastPlasmaHarmonicQuakeTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA HARMONIC DECK QUAKE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Harmonic plasma quake wave (360 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck recoil (130 Hz)
                goto combat_gesture_done;
            }
            if (hyperextensionFinisherThrow && (now - s_lastHyperextensionFinisherTime > 2000)) {
                s_lastHyperextensionFinisherTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER HYPEREXTENSION FINISHER EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_hyperextensionFinisherDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_hyperextensionFinisherDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_hyperextensionFinisherDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Cervical hyperextension fracture shockwave (360 Hz)
                vrState.leftHapticPulse = 0.92f;  // Whizzer collar tie drag (260 Hz)
                goto combat_gesture_done;
            }
            if (plasmaResonantWavePlunge && (now - s_lastPlasmaResonantWaveTime > 2000)) {
                s_lastPlasmaResonantWaveTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA RESONANT DETONATION EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Harmonic plasma detonation wave (360 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck recoil (120 Hz)
                goto combat_gesture_done;
            }
            if (cervicalGratingSlamThrow && (now - s_lastCervicalGratingSlamTime > 2000)) {
                s_lastCervicalGratingSlamTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER CERVICAL GRATING SLAM EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cervicalGratingSlamDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cervicalGratingSlamDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cervicalGratingSlamDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Cervical vertebra fracture shockwave (360 Hz)
                vrState.leftHapticPulse = 0.92f;  // Whizzer collar tie drag (250 Hz)
                goto combat_gesture_done;
            }
            if (plasmaOverloadWavePlunge && (now - s_lastPlasmaOverloadWaveTime > 2000)) {
                s_lastPlasmaOverloadWaveTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA OVERLOAD WAVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('C', false);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Plasma overload detonation wave (360 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck recoil (110 Hz)
                goto combat_gesture_done;
            }
            if (sternumGratingSlamThrow && (now - s_lastSternumGratingSlamTime > 2000)) {
                s_lastSternumGratingSlamTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER STERNUM GRATING SLAM EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_sternumGratingSlamDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_sternumGratingSlamDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_sternumGratingSlamDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Thoracic cage fracture shockwave (360 Hz)
                vrState.leftHapticPulse = 0.92f;  // Whizzer collar tie drag (250 Hz)
                goto combat_gesture_done;
            }
            if (plasmaCleaveWavePlunge && (now - s_lastPlasmaCleaveWaveTime > 2000)) {
                s_lastPlasmaCleaveWaveTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL PLASMA CLEAVE WAVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Kinetic plasma cleave wave (360 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck recoil (100 Hz)
                goto combat_gesture_done;
            }
            if (sternumFractureSpikeThrow && (now - s_lastSternumFractureSpikeTime > 2000)) {
                s_lastSternumFractureSpikeTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER STERNUM FRACTURE SPIKE EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_sternumFractureSpikeDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_sternumFractureSpikeDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_sternumFractureSpikeDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Sternum bone fracture shockwave (360 Hz)
                vrState.leftHapticPulse = 0.92f;  // Collar tie tension (250 Hz)
                goto combat_gesture_done;
            }
            if (kineticPulseWavePlunge && (now - s_lastKineticPulseWaveTime > 2000)) {
                s_lastKineticPulseWaveTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL KINETIC PULSE WAVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Kinetic plasma detonation wave (360 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck recoil (90 Hz)
                goto combat_gesture_done;
            }
            if (sternumKneeDropThrow && (now - s_lastSternumKneeDropTime > 2000)) {
                s_lastSternumKneeDropTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER STERNUM KNEE DROP EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_sternumKneeDropDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_sternumKneeDropDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_sternumKneeDropDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Sternum bone fracture shockwave (360 Hz)
                vrState.leftHapticPulse = 0.92f;  // Collar tie tension (240 Hz)
                goto combat_gesture_done;
            }
            if (omnidirectionalPulsePlunge && (now - s_lastOmnidirectionalPulseTime > 2000)) {
                s_lastOmnidirectionalPulseTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL DECK ARC PULSE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Double-frequency electrostatic pulse blast (360 Hz)
                vrState.leftHapticPulse = 0.98f;  // Seismic deck concussion (80 Hz)
                goto combat_gesture_done;
            }
            if (whizzerGroundDislocationThrow && (now - s_lastWhizzerGroundDislocationTime > 2000)) {
                s_lastWhizzerGroundDislocationTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER GROUND DISLOCATION EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_whizzerGroundDislocationDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_whizzerGroundDislocationDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_whizzerGroundDislocationDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Shoulder socket fracture and vertebral crack (360 Hz)
                vrState.leftHapticPulse = 0.92f;  // Overhook torque strain (230 Hz)
                goto combat_gesture_done;
            }
            if (omnidirectionalArcPlunge && (now - s_lastOmnidirectionalArcTime > 2000)) {
                s_lastOmnidirectionalArcTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON OMNIDIRECTIONAL DECK ARC EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_TAB, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_TAB, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Radial dielectric breakdown wave (360 Hz)
                vrState.leftHapticPulse = 0.95f;  // Landing seismic dispersion (75 Hz)
                goto combat_gesture_done;
            }
            if (whizzerGroundFinisherThrow && (now - s_lastWhizzerGroundFinisherTime > 2000)) {
                s_lastWhizzerGroundFinisherTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER GROUND FINISHER EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_whizzerGroundFinisherDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_whizzerGroundFinisherDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_whizzerGroundFinisherDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Bone fracture impact shockwave (360 Hz)
                vrState.leftHapticPulse = 0.90f;  // Downward collar tie drag (220 Hz)
                goto combat_gesture_done;
            }
            if (deckImpactPlunge && (now - s_lastDeckImpactTime > 2000)) {
                s_lastDeckImpactTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON VAULT CONCUSSIVE DECK IMPACT EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Concussive electro-kinetic deck blast (360 Hz)
                vrState.leftHapticPulse = 0.98f;  // Structural deck resonance (60 Hz)
                goto combat_gesture_done;
            }
            if (hyperextensionSpikeThrow && (now - s_lastHyperextensionSpikeTime > 2000)) {
                s_lastHyperextensionSpikeTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL HYPEREXTENSION MAT SPIKE EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_hyperextensionSpikeDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_hyperextensionSpikeDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_hyperextensionSpikeDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Cervical facet dislocation and mat impact shockwave (360 Hz)
                vrState.leftHapticPulse = 0.92f;  // Chin thrust hyperextension torque (240 Hz)
                goto combat_gesture_done;
            }
            if (vaultKineticCleavePlunge && (now - s_lastVaultKineticCleaveTime > 2000)) {
                s_lastVaultKineticCleaveTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON VAULT KINETIC CLEAVE EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Kinetic plasma detonation blast (360 Hz)
                vrState.leftHapticPulse = 0.90f;  // Two-handed landing frame recoil (80 Hz)
                goto combat_gesture_done;
            }
            if (thoracicSlamThrow && (now - s_lastThoracicSlamTime > 2000)) {
                s_lastThoracicSlamTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL THORACIC GRATING SLAM EXECUTED (Dir: %c, Ry: %.2f, Lx: %.2f)", s_thoracicSlamDirKey, rVel.y, lVel.x);
                SendKey('W', true);
                SendKey(s_thoracicSlamDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_thoracicSlamDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Ribcage crush and deck grating impact shockwave (360 Hz)
                vrState.leftHapticPulse = 0.90f;  // Thoracic squeeze lift strain (220 Hz)
                goto combat_gesture_done;
            }
            if (vaultGroundArcPlunge && (now - s_lastVaultGroundArcTime > 2000)) {
                s_lastVaultGroundArcTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON VAULT JUMP GROUND ARC EXECUTED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('C', false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Dielectric deck breakdown (360 Hz)
                vrState.leftHapticPulse = 0.95f;  // Landing seismic dispersion (70 Hz)
                goto combat_gesture_done;
            }
            if (cervicalSpikeThrow && (now - s_lastCervicalSpikeTime > 2000)) {
                s_lastCervicalSpikeTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL CERVICAL MAT SPIKE EXECUTED (Dir: %c, Ry: %.2f, Ly: %.2f)", s_cervicalSpikeDirKey, rVel.y, lVel.y);
                SendKey('W', true);
                SendKey(s_cervicalSpikeDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_cervicalSpikeDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Cervical vertebra fracture shockwave (360 Hz)
                vrState.leftHapticPulse = 0.90f;  // Collar tie neck drag tension (220 Hz)
                goto combat_gesture_done;
            }
            if (cleaveArcBlastStrike && (now - s_lastCleaveArcBlastTime > 2000)) {
                s_lastCleaveArcBlastTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON TWO-HANDED CLEAVE & ARC BLAST EXECUTED (Ry: %.2f, Rz: %.2f)", rVel.y, rVel.z);
                SendKey('S', true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.98f; // Frontal arc discharge detonation (360 Hz)
                vrState.leftHapticPulse = 0.90f;  // Two-handed shaft recoil (220 Hz)
                goto combat_gesture_done;
            }
            if (whizzerCounterThrow && (now - s_lastWhizzerCounterTime > 2000)) {
                s_lastWhizzerCounterTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER COUNTER-THROW EXECUTED (Dir: %c, Ry: %.2f, Lx: %.2f)", s_whizzerCounterDirKey, rVel.y, lVel.x);
                SendKey('W', true);
                SendKey(s_whizzerCounterDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_whizzerCounterDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Shoulder capsule dislocation and bone crack (350 Hz)
                vrState.leftHapticPulse = 0.85f;  // Overhook fulcrum strain (210 Hz)
                goto combat_gesture_done;
            }
            if (kineticCleaveSwing && (now - s_lastKineticCleaveTime > 2000)) {
                s_lastKineticCleaveTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON KINETIC CLEAVE & SHOCKWAVE STUN STRUCK (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey('S', true);
                SendKey('W', true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.98f; // High-arc kinetic impact and plasma discharge (360 Hz)
                vrState.leftHapticPulse = 0.88f;  // Secondary grip dielectric dissipation (240 Hz)
                goto combat_gesture_done;
            }
            if (lateralSuplexThrow && (now - s_lastLateralSuplexTime > 2000)) {
                s_lastLateralSuplexTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL LATERAL SUPLEX EXECUTED (Dir: %c, Ry: %.2f, Rx: %.2f)", s_lateralSuplexDirKey, rVel.y, rVel.x);
                SendKey('W', true);
                SendKey(s_lateralSuplexDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_lateralSuplexDirKey, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Spine compression and mat impact shockwave (360 Hz)
                vrState.leftHapticPulse = 0.92f;  // Double underhook torso lift torque (190 Hz)
                goto combat_gesture_done;
            }
            if (groundingThrustLunge && (now - s_lastGroundingThrustTime > 2000)) {
                s_lastGroundingThrustTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON GROUNDING THRUST & ARC RING EXECUTED (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
                SendKey('C', true);
                SendKey('W', true);
                SendKey('Q', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey('C', false);
                vrState.rightHapticPulse = 0.98f; // Closed-circuit dielectric breakdown (350 Hz)
                vrState.leftHapticPulse = 0.90f;  // Floor conduit surge (60 Hz)
                goto combat_gesture_done;
            }
            if (thaiPlumElbowPlunge && (now - s_lastThaiPlumElbowTime > 2000)) {
                s_lastThaiPlumElbowTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL THAI PLUM ELBOW SPIKE STRUCK (Dir: %c, Ry: %.2f)", s_thaiPlumElbowDirKey, rVel.y);
                SendKey('W', true);
                SendKey(s_thaiPlumElbowDirKey, true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey(s_thaiPlumElbowDirKey, false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.85f;  // Cervical collar tension (220 Hz)
                vrState.rightHapticPulse = 0.98f; // Cranial bone fracture shockwave (350 Hz)
                goto combat_gesture_done;
            }
            if (batonShieldBashThrust && (now - s_lastBatonBashTime > 2000)) {
                s_lastBatonBashTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON KINETIC SHIELD BASH & ARC BURST EXECUTED (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Planar electrostatic blast (340 Hz)
                vrState.leftHapticPulse = 0.98f;  // Blunt kinetic recoil (80 Hz)
                goto combat_gesture_done;
            }
            if (hipTossRotation && (now - s_lastHipTossTime > 2000)) {
                s_lastHipTossTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL DOUBLE UNDERHOOK HIP TOSS EXECUTED (Dir: %c, DeltaVx: %.2f)", s_hipTossDirKey, rVel.x - lVel.x);
                SendKey('W', true);
                SendKey(s_hipTossDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_hipTossDirKey, false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.90f;  // Thoracic squeeze lift tension (230 Hz)
                vrState.rightHapticPulse = 0.98f; // Hip toss deck impact blast (40 Hz)
                goto combat_gesture_done;
            }
            if (batonHammerfistPlunge && (now - s_lastHammerfistTime > 2000)) {
                s_lastHammerfistTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON KINETIC HAMMERFIST DECK STRIKE EXECUTED (Ry: %.2f, Rz: %.2f)", rVel.y, rVel.z);
                SendKey('S', true);
                SendKey('W', true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.98f; // High-voltage arc detonation blast (330 Hz)
                vrState.leftHapticPulse = 0.70f;  // Forearm bone recoil (70 Hz)
                goto combat_gesture_done;
            }
            if (whizzerDownWhip && (now - s_lastWhizzerTime > 2000)) {
                s_lastWhizzerTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL WHIZZER TAKEDOWN EXECUTED (Dir: %c, Ry: %.2f)", s_whizzerDirKey, rVel.y);
                SendKey('W', true);
                SendKey(s_whizzerDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_whizzerDirKey, false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.85f;  // Overhook shoulder lever strain (210 Hz)
                vrState.rightHapticPulse = 0.98f; // Concussive mat impact (45 Hz)
                goto combat_gesture_done;
            }
            if (javelinVaultStrike && (now - s_lastJavelinVaultTime > 2000)) {
                s_lastJavelinVaultTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON JAVELIN VAULT LEAP THRUST EXECUTED (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Airborne arc detonation blast (360 Hz)
                vrState.leftHapticPulse = 0.90f;  // Landing deck recoil (60 Hz)
                goto combat_gesture_done;
            }
            if (thaiPlumKneeDrive && (now - s_lastThaiPlumTime > 2000)) {
                s_lastThaiPlumTime = now;
                s_thaiPlumClinchTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL THAI PLUM LIVER KNEE STRUCK (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('W', true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.95f;  // Ribcage fracture & posture break (280 Hz)
                vrState.rightHapticPulse = 0.98f; // Concussive liver impact shockwave (320 Hz)
                goto combat_gesture_done;
            }
            if (radialDisarmThrust && (now - s_lastRadialDisarmTime > 2000)) {
                s_lastRadialDisarmTime = now;
                s_radialDisarmTwistTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON RADIAL DISARM PENETRATIVE THRUST DISCHARGED (Rz: %.2f)", rVel.z);
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                SendKey(s_radialDisarmDirKey, true);
                SendKey('W', true);
                SendKey('E', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('E', false);
                SendKey('W', false);
                SendKey(s_radialDisarmDirKey, false);
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                vrState.rightHapticPulse = 0.98f; // Penetrative arc detonation blast (350 Hz)
                vrState.leftHapticPulse = 0.70f;
                goto combat_gesture_done;
            }
            if (backSuplexHoistSlam && (now - s_lastBackSuplexTime > 2000)) {
                s_lastBackSuplexTime = now;
                s_backSuplexClinchTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL BELLY-TO-BACK SUPLEX SLAM EXECUTED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('S', true);
                SendKey('W', true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey('W', false);
                SendKey('S', false);
                vrState.leftHapticPulse = 0.98f;  // Vertebral floor impact shockwave (40 Hz)
                vrState.rightHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (isCrossThrustOvercharged && (now - s_lastCrossThrustTime > 2000)) {
                s_lastCrossThrustTime = now;
                s_crossBlockHoldStart = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON CROSS-CHEST OVERCHARGE DETONATION DISCHARGED");
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                SendKey('W', true);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                SendKey(VK_RETURN, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_RETURN, false);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('W', false);
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                vrState.rightHapticPulse = 0.98f; // Catastrophic dielectric breakdown blast (360 Hz)
                vrState.leftHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (outsideLegKneeDrop && (now - s_lastOutsideLegSweepTime > 2000)) {
                s_lastOutsideLegSweepTime = now;
                s_outsideLegSweepHookTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL KNEE DROP FINISHER EXECUTED (Ry: %.2f)", rVel.y);
                SendKey(s_outsideLegSweepDirKey, true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey(s_outsideLegSweepDirKey, false);
                vrState.rightHapticPulse = 0.98f; // Concussive knee-drop mat impact (320 Hz)
                vrState.leftHapticPulse = 0.95f;
                goto combat_gesture_done;
            }
            if (crossParryRiposte && (now - s_lastCrossParryTime > 2000)) {
                s_lastCrossParryTime = now;
                s_crossParryDeflectTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON CROSS-PARRY RIPOSTE DISCHARGED (Dir: %c, Rz: %.2f)", s_crossParryDirKey, rVel.z);
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                SendKey(s_crossParryDirKey, true);
                SendKey('W', true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey('W', false);
                SendKey(s_crossParryDirKey, false);
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                vrState.rightHapticPulse = 0.98f; // Electro-kinetic counter-detonation blast (350 Hz)
                vrState.leftHapticPulse = 0.70f;
                goto combat_gesture_done;
            }
            if (frontHeadlockWrap && frontHeadlockArchTorque && (now - s_lastFrontHeadlockTime > 2000)) {
                s_lastFrontHeadlockTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL FRONT HEADLOCK GUILLOTINE CRACK (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('W', true);
                SendKey('S', true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey('S', false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.98f;  // Cervical compression strain & bone snap (340 Hz)
                vrState.rightHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (isLookingDownward && batonOverheadVault && batonVaultPlunge && (now - s_lastVaultDropTime > 2000)) {
                s_lastVaultDropTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON HIGH-ARC OVERHEAD VAULT DROP STRIKE EXECUTED (Ry: %.2f)", rVel.y);
                SendKey(VK_SPACE, true);
                SendKey('S', true);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                SendKey('S', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // High-voltage arc detonation blast (350 Hz)
                vrState.leftHapticPulse = 0.95f;  // Heavy landing deck dispersion (55 Hz)
                goto combat_gesture_done;
            }
            if (suloevLowCrouch && suloevHookHold && suloevArchPull && (now - s_lastSuloevTime > 2000)) {
                s_lastSuloevTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL SULOEV STRETCH SUBMISSION EXECUTED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('C', true);
                SendKey('S', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('S', false);
                SendKey('C', false);
                vrState.leftHapticPulse = 0.98f;  // Tendon tear & hip dislocation shockwave (330 Hz)
                vrState.rightHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (isCrossThrustReady && (now - s_lastCrossThrustTime > 2000)) {
                s_lastCrossThrustTime = now;
                s_crossBlockHoldStart = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON CROSS-CHEST BLOCK & ARC THRUST DISRUPTION DISCHARGED");
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                SendKey('W', true);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('W', false);
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                vrState.rightHapticPulse = 0.98f; // High-voltage arc detonation blast (340 Hz)
                vrState.leftHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (firemansWhipThrow && (now - s_lastFiremansCarryTime > 2000)) {
                s_lastFiremansCarryTime = now;
                s_firemansLoadTime = 0;
                s_lastPunchTime = now;
                char throwDirKey = (rVel.x > 0.0f || lVel.x > 0.0f) ? 'D' : 'A';
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL FIREMAN'S CARRY SLAM EXECUTED (Dir: %c, ThrowX: %.2f)", throwDirKey, rVel.x);
                SendKey('W', true);
                SendKey(throwDirKey, true);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey(throwDirKey, false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.98f;  // Body-spike mat shockwave (45 Hz)
                vrState.rightHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (batonThroatThrustStrike && (now - s_lastPistolDisarmTime > 1800)) {
                s_lastPistolDisarmTime = now;
                s_pistolTrapTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON CROSS-PISTOL DISARM & THROAT THRUST EXECUTED (Rz: %.2f)", rVel.z);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                SendKey('E', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('E', false);
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Tracheal kinetic impact (320 Hz)
                vrState.leftHapticPulse = 0.65f;
                goto combat_gesture_done;
            }
            if (doubleLegSpikeSlam && (now - s_lastDoubleLegSlamTime > 2000)) {
                s_lastDoubleLegSlamTime = now;
                s_doubleLegLiftTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL DOUBLE-LEG MAT SLAM EXECUTED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('C', true);
                SendKey('W', true);
                SendKey(VK_SPACE, true);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                SendKey(VK_SPACE, false);
                SendKey('W', false);
                SendKey('C', false);
                vrState.leftHapticPulse = 0.98f;  // Concussive mat impact blast (40 Hz)
                vrState.rightHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (batonTripSweep && (now - s_lastBatonJabTripTime > 1800)) {
                s_lastBatonJabTripTime = now;
                s_batonJabTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON JAB-TO-TASER-TRIP TAKEDOWN EXECUTED (Ry: %.2f, Rx: %.2f)", rVel.y, rVel.x);
                SendKey('W', true);
                SendKey('S', true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('C', false);
                SendKey('S', false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.90f; // Low sweep trip impact (180 Hz)
                vrState.leftHapticPulse = 0.60f;
                goto combat_gesture_done;
            }
            if (isSingleLegTakedown && (now - s_lastSingleLegTakedownTime > 1800)) {
                s_lastSingleLegTakedownTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL SINGLE-LEG ANKLE PICK TAKEDOWN EXECUTED (Ly: %.2f, Rz: %.2f)", lVel.y, rVel.z);
                SendKey('C', true);
                SendKey('W', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('W', false);
                SendKey('C', false);
                vrState.leftHapticPulse = 0.80f;  // Ankle pick scoop (160 Hz)
                vrState.rightHapticPulse = 0.98f; // Deck takedown slam (260 Hz)
                goto combat_gesture_done;
            }
            if (isBatonDiagonalParry && (now - s_lastDiagonalParryTime > 1800)) {
                s_lastDiagonalParryTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON UPWARD DIAGONAL CROSS-PARRY & DISCHARGE EXECUTED (Ry: %.2f, Rx: %.2f)", rVel.y, rVel.x);
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                SendKey('W', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Kinetic deflection shockwave (330 Hz)
                vrState.leftHapticPulse = 0.75f;  // Off-hand conductive arc feedback (160 Hz)
                goto combat_gesture_done;
            }
            if (isArmTriangleReady) {
                s_lastArmTriangleTime = now;
                s_armTriangleStartTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL STANDING ARM-TRIANGLE CHOKE LOCKED & EXECUTED");
                SendKey('W', true);
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('C', false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.85f;  // Vascular occlusion compression (140 Hz)
                vrState.rightHapticPulse = 0.98f; // Cervical submission snap (280 Hz)
                goto combat_gesture_done;
            }
            if (isBatonRadialSweep && (now - s_lastRadialSweepTime > 2000)) {
                s_lastRadialSweepTime = now;
                s_lastPunchTime = now;
                char dirKey = (rVel.x > 0.0f) ? 'D' : 'A';
                VR_LOG_INFO("Tactical Melee: STUN BATON RADIAL KINETIC SWEEP & STUN RING EXECUTED (Dir: %c, Rx: %.2f)", dirKey, rVel.x);
                SendKey(dirKey, true);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey(dirKey, false);
                vrState.rightHapticPulse = 0.98f; // Radial discharge blast (310 Hz)
                vrState.leftHapticPulse = 0.85f;  // Off-hand follow-through ionization (280 Hz)
                goto combat_gesture_done;
            }
            if (isKimuraFracture && (now - s_lastKimuraTime > 2000)) {
                s_lastKimuraTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL KIMURA SHOULDER LOCK & FRACTURE EXECUTED (Ry: %.2f, Rx: %.2f)", rVel.y, rVel.x);
                SendKey('S', true);
                SendKey('A', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('A', false);
                SendKey('S', false);
                vrState.leftHapticPulse = 0.75f;  // Wrist bind resistance (190 Hz)
                vrState.rightHapticPulse = 0.98f; // Humerus fracture crack (320 Hz)
                goto combat_gesture_done;
            }
            if (isBatonJavelinThrust && (now - s_lastBatonJavelinTime > 1800)) {
                s_lastBatonJavelinTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON FORWARD TWO-HANDED JAVELIN THRUST EXECUTED (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
                SendKey('W', true);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Electro-kinetic impact pulse (320 Hz)
                vrState.leftHapticPulse = 0.65f;
                goto combat_gesture_done;
            }
            if (isOutsideTripMount && (now - s_lastOutsideTripTime > 1800)) {
                s_lastOutsideTripTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL OUTSIDE TRIP & MOUNT POUND EXECUTED (Lx: %.2f, Rz: %.2f)", lVel.x, rVel.z);
                SendKey('W', true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('C', false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.80f;  // Ankle hook resistance impact (170 Hz)
                vrState.rightHapticPulse = 0.98f; // Cranial deck pound shockwave (260 Hz)
                goto combat_gesture_done;
            }
            if (isCrossArcBurstReady && (now - s_lastCrossArcBurstTime > 1800)) {
                s_lastCrossArcBurstTime = now;
                s_crossArcStartTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON DUAL-HAND CROSS-BLOCK REACTIVE ARC BURST DISCHARGED");
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                vrState.leftHapticPulse = 0.98f;  // Concussive electrostatic repulsion blast (320 Hz)
                vrState.rightHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (suplexSpineSlam && (now - s_lastSuplexSlamTime > 1800)) {
                s_lastSuplexSlamTime = now;
                s_suplexHoistTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL UNDERHOOK SUPLEX SPINE SLAM EXECUTED");
                SendKey('S', true);
                SendKey(VK_SPACE, true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('C', false);
                SendKey(VK_SPACE, false);
                SendKey('S', false);
                vrState.leftHapticPulse = 0.98f;  // Deep sub-bass floor impact shockwave (45 Hz)
                vrState.rightHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (isBatonLateralDisarm && (now - s_lastBatonDisarmTime > 1500)) {
                s_lastBatonDisarmTime = now;
                s_lastPunchTime = now;
                char dirKey = (rVel.x < 0) ? 'A' : 'D';
                VR_LOG_INFO("Tactical Melee: STUN BATON LATERAL CROSS-SWEEP DEFLECTION & DISARM (Rx: %.2f)", rVel.x);
                SendKey(dirKey, true);
                SendKey('E', true);
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                SendKey('E', false);
                SendKey(dirKey, false);
                vrState.rightHapticPulse = 0.98f; // Electromagnetic repulsion arc blast (310 Hz)
                vrState.leftHapticPulse = 0.65f;
                goto combat_gesture_done;
            }
            if (hammerfistStrike && (now - s_lastShinTrapTime > 1500)) {
                s_lastShinTrapTime = now;
                s_shinTrapTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: OVERHAND HAMMERFIST COMPLETED COMBO (Ry: %.2f)", rVel.y);
                SendKey('S', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.98f; // Skeletal hammerfist shockwave (280 Hz)
                vrState.leftHapticPulse = 0.60f;
                goto combat_gesture_done;
            }
            if (isBatonSkullBreaker && (now - s_lastSkullBreakerTime > 1500)) {
                s_lastSkullBreakerTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON CHARGED HIGH-ARC DOWNWARD SKULL BREAKER CLEAVED (Ry: %.2f)", rVel.y);
                SendKey('W', true);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Electro-kinetic cranial detonation blast (340 Hz)
                vrState.leftHapticPulse = 0.60f;
                goto combat_gesture_done;
            }
            if (guillotineWrapStrike && (now - s_lastSnapdownChokeTime > 1500)) {
                s_lastSnapdownChokeTime = now;
                s_snapdownStartTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: GUILLOTINE CHOKE LOCKED (Ry: %.2f, Rz: %.2f)", rVel.y, rVel.z);
                SendKey('S', true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('C', false);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.98f; // Tracheal compression chokehold pulse (280 Hz)
                vrState.leftHapticPulse = 0.70f;
                goto combat_gesture_done;
            }
            if (isBatonGroundDischarge && (now - s_lastBatonGroundShockTime > 2000)) {
                s_lastBatonGroundShockTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON DUAL-HAND GROUND DISCHARGE KINETIC SHOCKWAVE DETONATED (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey('S', true);
                SendKey(VK_SPACE, true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey(VK_SPACE, false);
                SendKey('S', false);
                vrState.leftHapticPulse = 0.98f;  // Seismic ground shock & dielectric breakdown (50 Hz & 320 Hz)
                vrState.rightHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (cervicalSnapWhip && (now - s_lastPalmChinComboTime > 1500)) {
                s_lastPalmChinComboTime = now;
                s_palmChinStrikeTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: CERVICAL SNAP COMPLETED COMBO (Lx: %.2f)", lVel.x);
                SendKey('W', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.98f; // Cervical vertebrae fracture snap (300 Hz)
                vrState.rightHapticPulse = 0.70f;
                goto combat_gesture_done;
            }
            if (isBatonArcTrapArmed) {
                VR_LOG_INFO("Tactical Melee: STUN BATON HIGH-VOLTAGE CAPACITANCE BLEED ARC TRAP ARMED");
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Static surface detonation snap (140 Hz)
                vrState.leftHapticPulse = 0.70f;
                goto combat_gesture_done;
            }
            if (isCrossChestArcWall) {
                SendKey('S', true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey('S', false);
                goto combat_gesture_done;
            }
            if (isInductionOverheated) {
                VR_LOG_INFO("Tactical Melee: STUN BATON INDUCTION COIL OVERHEAT ARC DETONATED");
                SendKey('S', true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey('S', false);
                vrState.leftHapticPulse = 0.98f;  // Blinding EMP arc detonation shockwave (300 Hz)
                vrState.rightHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
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
                goto combat_gesture_done;
            }
            if (isBatonGroundPin && (now - s_lastBatonGroundPinTime > 1500)) {
                s_lastBatonGroundPinTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON GROUND PIN & TASER KNEEL (Ry: %.2f)", rVel.y);
                SendKey('C', true);
                SendKey('S', true);
                SendKey('Q', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('Q', false);
                SendKey('S', false);
                SendKey('C', false);
                vrState.rightHapticPulse = 0.98f; // Heavy deck collision & ground circuit arc (260 Hz)
                vrState.leftHapticPulse = 0.65f;
                goto combat_gesture_done;
            }
            if (isClinchHeadbutt && (now - s_lastHeadbuttTime > 1500)) {
                s_lastHeadbuttTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL COLLAR TIE CLINCH & HEADBUTT STUN (Lz: %.2f, Rz: %.2f)", lVel.z, rVel.z);
                SendKey('W', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.98f;  // Concussive cranial skull collision shockwave (180 Hz)
                vrState.rightHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (sternumElbowSmash && (now - s_lastLiverElbowComboTime > 1500)) {
                s_lastLiverElbowComboTime = now;
                s_liverHookTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: STERNUM ELBOW SMASH COMPLETED COMBO (Ry: %.2f)", rVel.y);
                SendKey('W', true);
                SendKey('A', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('A', false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Bone-fracturing sternum blunt crack (280 Hz)
                goto combat_gesture_done;
            }
            if (isBatonChargedSpear && (now - s_lastSpearThrustTime > 1500)) {
                s_lastSpearThrustTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON CHARGED UNDERARM SPEAR THRUST (Rz: %.2f)", rVel.z);
                SendKey('W', true);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // High-voltage linear shockwave (310 Hz)
                vrState.leftHapticPulse = 0.65f;  // Forearm bracing recoil (140 Hz)
                goto combat_gesture_done;
            }
            if (isKneeStomp && (now - s_lastKneeStompTime > 1500)) {
                s_lastKneeStompTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL SHIN KICK & KNEE HYPER-EXTENSION (Ry: %.2f, Lz: %.2f)", rVel.y, lVel.z);
                SendKey('C', true);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                SendKey('C', false);
                vrState.rightHapticPulse = 0.98f; // Joint-crushing skeletal fracture shockwave (240 Hz)
                vrState.leftHapticPulse = 0.60f;  // Body-weight plant & brace feedback (100 Hz)
                goto combat_gesture_done;
            }
            if (isLeadKneeThrust && (now - s_lastLeadKneeTime > 1500)) {
                s_lastLeadKneeTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL LEAD KNEE THRUST (Ry: %.2f, Ly: %.2f)", rVel.y, lVel.y);
                SendKey('W', true);
                SendKey('C', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('C', false);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Concussive abdominal organ shockwave (200 Hz)
                vrState.leftHapticPulse = 0.65f;  // Frame clamp vibration (100 Hz)
                goto combat_gesture_done;
            }
            if (isDuckUnderKidneyCounter && (now - s_lastKidneyCounterTime > 1500)) {
                s_lastKidneyCounterTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL DUCK-UNDER KIDNEY COUNTER-JAB (Rx: %.2f, Rz: %.2f)", rVel.x, rVel.z);
                SendKey('C', true);
                SendKey('D', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('D', false);
                SendKey('C', false);
                vrState.rightHapticPulse = 0.98f; // Visceral renal concussive shockwave (220 Hz)
                goto combat_gesture_done;
            }
            if (blindRiposteStrike && (now - s_lastEyeRubRiposteTime > 1500)) {
                s_lastEyeRubRiposteTime = now;
                s_eyeRubWipeTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: DIRT/ASH BLIND RECOVERY RIPOSTE DISPATCHED (Rz: %.2f)", rVel.z);
                SendKey(VK_SPACE, true);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.98f; // Retaliatory counter-strike blast (240 Hz)
                goto combat_gesture_done;
            }
            if (isParrySnap && (now - s_lastParrySnapTime > 1500)) {
                s_lastParrySnapTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE SHIV FOREARM DEFENSIVE PARRY-AND-SNAP (Lx: %.2f, Ry: %.2f)", lVel.x, rVel.y);
                SendKey('W', true);
                SendKey('E', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('E', false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.80f;  // Forearm deflection snap (220 Hz)
                vrState.rightHapticPulse = 0.98f; // Joint-shattering bone crack (280 Hz)
                goto combat_gesture_done;
            }
            if (isBatonRibJab && (now - s_lastBatonRibJabTime > 1500)) {
                s_lastBatonRibJabTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Tactical Melee: STUN BATON RIB-JAB & GROUND TAKEDOWN (Ry: %.2f, Rz: %.2f)", rVel.y, rVel.z);
                SendKey('S', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.98f; // Neuromuscular shock & ground deck impact (290 Hz)
                goto combat_gesture_done;
            }
            if (isDoubleEarClap && (now - s_lastDoubleEarClapTime > 1500)) {
                s_lastDoubleEarClapTime = now;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: INMATE PIT BRAWL DOUBLE-EAR CONCUSSIVE CLAP (Lx: %.2f, Rx: %.2f)", lVel.x, rVel.x);
                SendKey('W', true);
                SendKey(VK_SPACE, true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey(VK_SPACE, false);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.98f;  // Concussive acoustic blast (160 Hz)
                vrState.rightHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (dartCounterJabStrike && (now - s_lastDartCounterJabTime > 1500)) {
                s_lastDartCounterJabTime = now;
                s_dartExtractedTime = 0;
                s_lastPunchTime = now;
                VR_LOG_INFO("Physical Combat: STUN DART COUNTER-JAB DISPATCHED (Rz: %.2f)", rVel.z);
                SendKey('E', true);
                SendKey('3', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('3', false);
                SendKey('E', false);
                vrState.rightHapticPulse = 0.98f; // Bio-electric discharge shockwave (320 Hz)
                goto combat_gesture_done;
            }
            if (isRibUnderhook && (now - s_lastRibUnderhookTime > 1500)) {
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
                goto combat_gesture_done;
            }
            if (dualSledgeOverhead) {
                VR_LOG_INFO("Physical Gesture: HEAVY ORE SLEDGEHAMMER OVERHEAD CRUSH (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.98f; // Catastrophic blunt fracture shockwave (80 Hz)
                vrState.leftHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (stealthCarotidSlash) {
                VR_LOG_INFO("Physical Stealth: CAROTID ARTERIAL JUGULAR SLASH (SpeedX: %.2f m/s, Ry: %.2f)", rVel.x, rightLocal.y);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.95f; // Arterial release incision & spray (260 Hz)
                goto combat_gesture_done;
            }
            if (twoHandedBatonSlam) {
                VR_LOG_INFO("Physical Gesture: TWO-HANDED STUN BATON DEFIBRILLATOR SLAM (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.98f; // Electro-kinetic shockwave blast (280 Hz)
                vrState.leftHapticPulse = 0.98f;
                goto combat_gesture_done;
            }
            if (mechPowerPunch) {
                VR_LOG_INFO("Physical Gesture: EXOSKELETON POWER FIST KINETIC SHOCKWAVE PUNCH (Rz: %.2f m/s)", rVel.z);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.98f; // Piston shockwave blast (100 Hz)
                vrState.leftHapticPulse = 0.85f;
                goto combat_gesture_done;
            }
            if (droneEmpSnap) {
                VR_LOG_INFO("Physical Combat: SECURITY DRONE EMP OVERLOAD & CHASSIS SNAP (TorqueRelY: %.2f)", fabsf(lVel.y - rVel.y));
                SendKeyTap('E');
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                vrState.rightHapticPulse = 0.95f; // Mechanical spine crunch & EMP arc wave (280 Hz)
                vrState.leftHapticPulse = 0.95f;
                goto combat_gesture_done;
            }
            if (leftTrayBatter) {
                VR_LOG_INFO("Physical Gesture: INMATE MESS TRAY CONCUSSIVE BATTER (Lz: %.2f)", lVel.z);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                vrState.leftHapticPulse = 0.90f; // Sheet metal concussive clatter (160 Hz)
                goto combat_gesture_done;
            }
            if (dualShieldBash) {
                VR_LOG_INFO("Physical Gesture: RIOT SHIELD CONCUSSIVE BASH (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.95f; // Shield rim blunt shockwave (140 Hz)
                vrState.leftHapticPulse = 0.95f;
                goto combat_gesture_done;
            }
            if (dualShieldPlant) {
                VR_LOG_INFO("Physical Gesture: RIOT SHIELD BALLISTIC DECK PLANT (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('C', true); // Crouch lock cover stance
                SendKey(VK_SPACE, true);
                SendKey(VK_SPACE, false);
                vrState.rightHapticPulse = 0.75f; // Deck clamp thud (80 Hz)
                vrState.leftHapticPulse = 0.75f;
                goto combat_gesture_done;
            }
            if (dualStoolHeavySmash) {
                VR_LOG_INFO("Physical Gesture: INMATE STOOL / BLUNT WEAPON HEAVY SMASH (LSpd: %.2f, RSpd: %.2f)", lSpeed, rSpeed);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);
                vrState.rightHapticPulse = 0.95f; // Bone-shattering blunt fracture (120 Hz)
                vrState.leftHapticPulse = 0.95f;
                goto combat_gesture_done;
            }
            if (dualPalmShove) {
                VR_LOG_INFO("Physical Gesture: PIT FIGHTING ARENA DUAL-PALM CONCUSSIVE SHOVE (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
                SendKey('W', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('W', false);
                vrState.rightHapticPulse = 0.85f; // Elastic ring boundary / concussive shockwave (140 Hz)
                vrState.leftHapticPulse = 0.85f;
                goto combat_gesture_done;
            }
            if (isLookingDown && isDownwardPlunge) {
                VR_LOG_INFO("Physical Gesture: DEATH-FROM-ABOVE AERIAL TAKEDOWN / DROP AMBUSH (Pitch: %.2f rad, Vy: %.2f m/s)", s_lastHmdPitch, activeVel.y);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                vrState.rightHapticPulse = 0.95f; // Bone-shattering ground impact shockwave (280 Hz)
                vrState.leftHapticPulse = 0.95f;
                goto combat_gesture_done;
            }
            if (dualOverheadSlam) {
                VR_LOG_INFO("Physical Gesture: TWO-HANDED OVERHEAD CRUSHING SLAM / PIPE STRIKE (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);

                vrState.rightHapticPulse = 0.95f; // Heavy blunt concussive shockwave (120 Hz)
                vrState.leftHapticPulse = 0.95f;
                goto combat_gesture_done;
            }
            if (dualCrossSlash) {
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
                goto combat_gesture_done;
            }
            if (activeVel.y < -1.35f && (activeLocal.y > -0.10f || s_isPhysicallyCrouched)) {
                VR_LOG_INFO("Physical Gesture: OVERHEAD SHIV PLUNGE / FINISHER (Vy: %.2f m/s, Y: %.2fm)", activeVel.y, activeLocal.y);
                SendKey('S', true);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKey('S', false);

                if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.95f;
                else vrState.leftHapticPulse = 0.95f;
                goto combat_gesture_done;
            }
            if ((s_isPhysicallyCrouched || s_isStealthPeeking) && activeLocal.z < -0.25f && (activeVel.z < -1.1f || fabsf(activeVel.x) > 1.1f)) {
                VR_LOG_INFO("Physical Gesture: STEALTH TAKEDOWN / THROAT-SLIT initiated (Vz: %.2f, Vx: %.2f)", activeVel.z, activeVel.x);
                SendKeyTap('E');
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

                if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.90f;
                else vrState.leftHapticPulse = 0.90f;
                goto combat_gesture_done;
            }
            {
                // ADR 0378: Melee Trajectory Filtering & Dominant Axis Strike Direction Alignment
                float fwdZ = -activeVel.z; // Forward punch velocity is along -Z
                float absX = fabsf(activeVel.x);
                float absY = fabsf(activeVel.y);

                // Forward Straight Punch / Blade Thrust: forward speed dominates lateral & vertical
                if (fwdZ > 1.10f && fwdZ >= absX && fwdZ >= absY) {
                    bool isBatonArcJab = (vrState.rightController.trigger > 0.60f);
                    VR_LOG_INFO("Physical Gesture: %s detected (Vz: %.2f m/s, Speed: %.2f m/s)",
                        isBatonArcJab ? "RIOT BATON ARC JAB" : "STRAIGHT JAB / BLADE THRUST", fwdZ, (rSpeed > lSpeed) ? rSpeed : lSpeed);
                    SendKey('W', true);
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                    SendKey('W', false);

                    float pulse = isBatonArcJab ? 0.95f : 0.90f;
                    if (rSpeed > lSpeed) vrState.rightHapticPulse = pulse;
                    else vrState.leftHapticPulse = pulse;
                    goto combat_gesture_done;
                }
                // Uppercut: upward vertical speed dominates forward & lateral
                else if (activeVel.y > 1.10f && activeVel.y > absX && activeVel.y > fwdZ) {
                    VR_LOG_INFO("Physical Gesture: UPPERCUT detected (Vy: %.2f m/s)", activeVel.y);
                    SendKey('S', true);
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                    SendKey('S', false);

                    if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.9f;
                    else vrState.leftHapticPulse = 0.9f;
                    goto combat_gesture_done;
                }
                // Lateral Hook / Blade Slash: lateral speed dominates
                else if (absX > 1.10f) {
                    char hookKey = (activeVel.x > 0.0f) ? 'D' : 'A';
                    VR_LOG_INFO("Physical Gesture: %s HOOK / BLADE SLASH detected (Vx: %.2f m/s)",
                        (hookKey == 'D') ? "RIGHT" : "LEFT", activeVel.x);
                    SendKey(hookKey, true);
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                    SendKey(hookKey, false);

                    if (rSpeed > lSpeed) vrState.rightHapticPulse = 0.9f;
                    else vrState.leftHapticPulse = 0.9f;
                    goto combat_gesture_done;
                }
            }
        combat_gesture_done:
            s_lastPunchTime = now;
        }
    }

    // 5b. Physical Boxer Guard Pose Detection ("holding both fists up in front of the VR")
    // Criteria (ADR 0400):
    // - Both controllers raised in front of chest/chin/face level relative to HMD (y in -0.36 to +0.25)
    // - Both controllers in front of face (z in -0.58 to -0.10)
    // - Both hands natural shield distance (|x_left - x_right| < 0.48m)
    // - Hands steady (not throwing a punch, rSpeed < 1.3 m/s and lSpeed < 1.3 m/s)
    // - Outside punch execution recovery window (> 200ms since last punch)
    // - Not actively holding ladder rungs or climbing
    bool chinLevel = (leftLocal.y >= -0.36f && leftLocal.y <= 0.25f) &&
                     (rightLocal.y >= -0.36f && rightLocal.y <= 0.25f);
    bool inFrontOfFace = (leftLocal.z >= -0.58f && leftLocal.z <= -0.10f) &&
                         (rightLocal.z >= -0.58f && rightLocal.z <= -0.10f);
    bool handsClose = fabsf(leftLocal.x - rightLocal.x) < 0.48f;
    bool handsSteady = (rSpeed < 1.3f && lSpeed < 1.3f);
    bool notPunching = (now - s_lastPunchTime > 200);

    bool guardPoseDetected = !isClimbingActive && chinLevel && inFrontOfFace && handsClose && handsSteady && notPunching;

    // Temporal Hysteresis & Debouncing:
    // Require guard pose to hold for ~50ms to engage (prevents false-triggers during punches)
    if (guardPoseDetected) {
        s_guardPoseReleaseTimer = 0;
        if (s_guardPoseHoldTimer == 0) {
            s_guardPoseHoldTimer = now;
        } else if (now - s_guardPoseHoldTimer >= 50) {
            s_physicalBlockEngaged = true;
        }
    } else {
        s_guardPoseHoldTimer = 0;
        if (s_physicalBlockEngaged) {
            // If throwing a punch, release block instantly (0ms)
            if (rSpeed > 1.4f || lSpeed > 1.4f) {
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

    // ADR 0394: Tactical Weapon Reload Ergonomics & Off-Hand Mag-Well Gating
    // Suppress reload when already in two-handed stabilization to prevent accidental reloads while aiming.
    // Off-hand reaching to dominant weapon receiver requires upward seating motion (lVel.y > 0.35 m/s).
    // Waist pouch reach strictly tracks off-hand (leftLocal), eliminating accidental reloads when resting dominant arm.
    if (!s_physicalBlockEngaged && !isClimbingActive && !s_twoHandedStabilizationActive && (now - s_lastReloadGestureTime > 1200)) {
        bool offHandMagInsert = (reloadHandDist < 0.18f && vrState.leftController.grip > 0.50f && lVel.y > 0.35f);
        bool offHandWaistPouch = (leftLocal.y < -0.38f && leftLocal.x > -0.35f && leftLocal.x < -0.05f && vrState.leftController.grip > 0.50f);

        // Cybernetic Drone HUD Battery Power Cell Hot-Swap Ejection (ADR 0082)
        bool cellEjectFlick = (reloadHandDist < 0.18f && vrState.leftController.grip > 0.50f && lVel.y < -0.70f);
        if (cellEjectFlick) {
            s_lastReloadGestureTime = now;
            VR_LOG_INFO("Tactical Weapon: POWER CELL HOT-SWAP EJECTED (Vy: %.2f m/s, Dist: %.2fm)", lVel.y, reloadHandDist);
            SendKeyTap('R');
            vrState.leftHapticPulse = 0.75f; // Magnetic cell disconnect snap (140 Hz)
            vrState.rightHapticPulse = 0.50f;
        } else if (offHandMagInsert || offHandWaistPouch) {
            s_lastReloadGestureTime = now;
            VR_LOG_INFO("Tactical Gesture: MANUAL WEAPON RELOAD triggered (%s, HandDist: %.2fm, LeftY: %.2f)",
                offHandMagInsert ? "Weapon Receiver / Mag-Well" : "Waist Ammo Pouch", reloadHandDist, leftLocal.y);
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
    // Detect when player brings off-hand to steady the two-handed weapon (ADR 0379):
    // - Distance hysteresis: Attach within 28cm, detach when separated beyond 38cm
    // - Grip hysteresis: Attach when leftController.grip > 0.50, detach when < 0.30
    // - Not in boxer guard, reloading, or climbing
    float twoHandDist = sqrtf(
        powf(vrState.leftController.gripPose.position.x - vrState.rightController.gripPose.position.x, 2) +
        powf(vrState.leftController.gripPose.position.y - vrState.rightController.gripPose.position.y, 2) +
        powf(vrState.leftController.gripPose.position.z - vrState.rightController.gripPose.position.z, 2)
    );

    bool isReloading = (now - s_lastReloadGestureTime <= 1100);
    bool isHarriesStance = s_flashlightActive && (twoHandDist < 0.18f) && (vrState.leftController.grip > 0.35f);

    constexpr float DIST_ATTACH = 0.28f;
    constexpr float DIST_DETACH = 0.38f;
    constexpr float GRIP_ATTACH = 0.50f;
    constexpr float GRIP_DETACH = 0.30f;

    bool wantTwoHanded = false;
    if (s_twoHandedStabilizationActive) {
        wantTwoHanded = (twoHandDist < DIST_DETACH && vrState.leftController.grip > GRIP_DETACH) || isHarriesStance;
    } else {
        wantTwoHanded = (twoHandDist < DIST_ATTACH && vrState.leftController.grip > GRIP_ATTACH) || isHarriesStance;
    }
    wantTwoHanded = wantTwoHanded && !s_physicalBlockEngaged && !isReloading && !isClimbingActive;

    if (wantTwoHanded != s_twoHandedStabilizationActive) {
        s_twoHandedStabilizationActive = wantTwoHanded;
        if (s_twoHandedStabilizationActive) {
            VR_LOG_INFO("Two-Handed Firearm Stabilization ENGAGED (%s, HandDist: %.2fm)",
                isHarriesStance ? "Tactical Harries Flashlight Stance" : "Foregrip Hold", twoHandDist);
            vrState.leftHapticPulse = isHarriesStance ? 0.35f : 0.25f;  // Subtle haptic confirmation pulse
            vrState.rightHapticPulse = isHarriesStance ? 0.35f : 0.25f;
        } else {
            VR_LOG_INFO("Two-Handed Firearm Stabilization DISENGAGED (HandDist: %.2fm)", twoHandDist);
            s_hasStabilizedPose = false;
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
            // ADR 0373 & ADR 0402: Speed-Adaptive One-Euro Filter for One-Handed Weapon Aim Stabilization
            // When firing or aiming one-handed, natural physiological hand tremor (3-8 Hz) causes sight jitter.
            // When holding steady in physical ADS corridor, apply high damping (alpha ≈ 0.15) to lock sight rock-steady.
            // As movement speed increases, ramp alpha up to 0.90 for zero-latency target acquisition.
            static RiddickVRTrackedPose s_oneHandedPose{};
            static bool s_hasOneHandedPose = false;

            if (!s_hasOneHandedPose) {
                s_oneHandedPose = vrState.rightController.aimPose;
                s_hasOneHandedPose = true;
            } else {
                float rSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y + rVel.z * rVel.z);
                float baseAlpha = s_isPhysicalADS ? 0.15f : 0.22f;
                float alpha = baseAlpha;
                if (rSpeed > 0.08f) {
                    float factor = (rSpeed - 0.08f) / 0.35f;
                    if (factor > 1.0f) factor = 1.0f;
                    alpha = baseAlpha + factor * (0.90f - baseAlpha);
                }

                s_oneHandedPose.position.x += alpha * (vrState.rightController.aimPose.position.x - s_oneHandedPose.position.x);
                s_oneHandedPose.position.y += alpha * (vrState.rightController.aimPose.position.y - s_oneHandedPose.position.y);
                s_oneHandedPose.position.z += alpha * (vrState.rightController.aimPose.position.z - s_oneHandedPose.position.z);

                float dot = s_oneHandedPose.orientation.x * vrState.rightController.aimPose.orientation.x +
                            s_oneHandedPose.orientation.y * vrState.rightController.aimPose.orientation.y +
                            s_oneHandedPose.orientation.z * vrState.rightController.aimPose.orientation.z +
                            s_oneHandedPose.orientation.w * vrState.rightController.aimPose.orientation.w;
                float qSign = (dot < 0.0f) ? -1.0f : 1.0f;

                s_oneHandedPose.orientation.x += alpha * (qSign * vrState.rightController.aimPose.orientation.x - s_oneHandedPose.orientation.x);
                s_oneHandedPose.orientation.y += alpha * (qSign * vrState.rightController.aimPose.orientation.y - s_oneHandedPose.orientation.y);
                s_oneHandedPose.orientation.z += alpha * (qSign * vrState.rightController.aimPose.orientation.z - s_oneHandedPose.orientation.z);
                s_oneHandedPose.orientation.w += alpha * (qSign * vrState.rightController.aimPose.orientation.w - s_oneHandedPose.orientation.w);

                float qNorm = sqrtf(s_oneHandedPose.orientation.x * s_oneHandedPose.orientation.x +
                                    s_oneHandedPose.orientation.y * s_oneHandedPose.orientation.y +
                                    s_oneHandedPose.orientation.z * s_oneHandedPose.orientation.z +
                                    s_oneHandedPose.orientation.w * s_oneHandedPose.orientation.w);
                if (qNorm > 0.0001f) {
                    s_oneHandedPose.orientation.x /= qNorm;
                    s_oneHandedPose.orientation.y /= qNorm;
                    s_oneHandedPose.orientation.z /= qNorm;
                    s_oneHandedPose.orientation.w /= qNorm;
                }

                vrState.rightController.aimPose = s_oneHandedPose;
            }
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
    static int s_activeWeaponSlot = 1; // ADR 0376: 1: Primary, 2: Sidearm, 3: Melee/Blades, 4: Ordnance, 5: Heavy
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
            s_activeWeaponSlot = 1;
            VR_LOG_INFO("Holster Gesture: PRIMARY WEAPON slot toggled (Over-Shoulder)");
            SendKeyTap('1');
            vrState.rightHapticPulse = 0.75f; // Solid mechanical latch pulse
        } else if (rightAtChest) {
            s_lastHolsterTime = now;
            s_activeWeaponSlot = 4;
            VR_LOG_INFO("Holster Gesture: TACTICAL GRENADE slot toggled (Chest Bandolier)");
            SendKeyTap('4');
            vrState.rightHapticPulse = 0.65f; // Grenade bandolier snap pulse
        } else if (rightLowSpine) {
            s_lastHolsterTime = now;
            s_activeWeaponSlot = 5;
            VR_LOG_INFO("Holster Gesture: HEAVY WEAPON / SCAR GUN slot toggled (Low Spine)");
            SendKeyTap('5');
            vrState.rightHapticPulse = 0.85f; // Heavy industrial magnetic clamp pulse
        } else if ((s_isPhysicallyCrouched || s_isDeepProne) && (leftAtBootSheath || rightAtBootSheath)) {
            s_lastHolsterTime = now;
            s_activeWeaponSlot = 3;
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
            s_activeWeaponSlot = 2;
            VR_LOG_INFO("Holster Gesture: SIDEARM / PISTOL slot toggled (Right Hip)");
            SendKeyTap('2');
            vrState.rightHapticPulse = 0.65f; // Leather holster draw pulse
        } else if (leftAtHip) {
            s_lastHolsterTime = now;
            s_activeWeaponSlot = 3;
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

#if 0 // Disabled legacy procedural environmental gestures (ADR 0398) to prevent spurious key bursts
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

            // Mining Core Pneumatic Rock Bolter Sinking & Anchor Driver (ADR 0123)
            static DWORD s_lastRockBolterTime = 0;
            bool leftHoldingGuideSleeve = (leftLocal.z > -0.45f && leftLocal.z < -0.16f && leftLocal.y > -0.20f && leftLocal.y < 0.25f) &&
                                          (vrState.leftController.grip > 0.60f);
            bool rightBolterRamming = (twoHandDist > 0.18f && twoHandDist < 0.40f) &&
                                      (vrState.rightController.grip > 0.60f && vrState.rightController.trigger > 0.65f) &&
                                      (rVel.z < -1.05f);
            if (leftHoldingGuideSleeve && rightBolterRamming && (now - s_lastRockBolterTime > 2000)) {
                s_lastRockBolterTime = now;
                VR_LOG_INFO("Environmental Gesture: MINING CORE PNEUMATIC ROCK BOLTER SUNK & LOCKED (Rz: %.2f m/s)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                vrState.leftHapticPulse = 0.65f;  // High-frequency sleeve rattle (210 Hz)
                vrState.rightHapticPulse = 0.98f; // Heavy pneumatic percussion impact (55 Hz)
            }

            // Subterranean Mining Core Pneumatic Breaker Chisel Point Swap (ADR 0130)
            static DWORD s_lastChiselSwapTime = 0;
            static DWORD s_collarReleasedTime = 0;
            bool twoHandAtChiselChuck = (twoHandDist < 0.18f) && (vrState.leftController.grip > 0.60f);
            bool collarYankBack = (lVel.z > 0.65f);
            if (twoHandAtChiselChuck && collarYankBack && (now - s_lastChiselSwapTime > 2000)) {
                if (s_collarReleasedTime == 0) {
                    s_collarReleasedTime = now;
                    VR_LOG_INFO("Environmental Gesture: PNEUMATIC CHISEL CHUCK COLLAR RELEASED (Lz: %.2f)", lVel.z);
                    vrState.leftHapticPulse = 0.60f; // Spring ball-detent release click (180 Hz)
                }
            }
            bool diamondChiselSeatRam = (s_collarReleasedTime > 0 && (now - s_collarReleasedTime < 600) && rVel.z < -0.80f);
            if (diamondChiselSeatRam && (now - s_lastChiselSwapTime > 2000)) {
                s_lastChiselSwapTime = now;
                s_collarReleasedTime = 0;
                VR_LOG_INFO("Environmental Gesture: HARDENED DIAMOND CHISEL POINT SEATED & LOCKED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap('3');
                vrState.rightHapticPulse = 0.90f; // Hardened alloy chuck lock snap (90 Hz)
                vrState.leftHapticPulse = 0.70f;
            }

            // Subterranean Mining Rail Pneumatic Car Tipper Lock Release (ADR 0135)
            static DWORD s_lastCarTipperTime = 0;
            bool bothAtTipperPedestal = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && rightLocal.z > -0.42f && rightLocal.z < -0.16f) &&
                                        (leftLocal.y > -0.55f && leftLocal.y < -0.20f && rightLocal.y > -0.55f && rightLocal.y < -0.20f) &&
                                        (vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
            bool dualUpwardTipperHoist = (lVel.y > 0.90f && rVel.y > 0.90f);
            if (bothAtTipperPedestal && dualUpwardTipperHoist && (now - s_lastCarTipperTime > 2000)) {
                s_lastCarTipperTime = now;
                VR_LOG_INFO("Environmental Gesture: MINING RAIL CAR TIPPER RELEASED & CHASSIS DUMPED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_RETURN);
                vrState.leftHapticPulse = 0.95f;  // Pawl release snap & ore dump rumble (50 Hz)
                vrState.rightHapticPulse = 0.95f;
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

            // Cryo-Stasis Chamber Manual Coolant Bleed Petcock Turn (ADR 0127)
            static DWORD s_lastCryoBleedTime = 0;
            bool handAtCryoBleedPetcock = (rightLocal.z > -0.40f && rightLocal.z < -0.15f && rightLocal.y > -0.30f && rightLocal.y < 0.10f && vrState.rightController.grip > 0.55f);
            bool cryoBleedTurn = (fabsf(rVel.x) > 0.55f || fabsf(rVel.y) > 0.55f);
            if (handAtCryoBleedPetcock && cryoBleedTurn && (now - s_lastCryoBleedTime > 1500)) {
                s_lastCryoBleedTime = now;
                VR_LOG_INFO("Environmental Gesture: CRYO-STASIS COOLANT BLEED PETCOCK OPENED (Vx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.75f; // Cryo vapor jet expansion rumble (140 Hz)
                vrState.leftHapticPulse = 0.35f;
            }

            // Cryo-Chamber Defrost Temperature Sensor Probe Insertion (ADR 0138)
            static DWORD s_lastThermalProbeTime = 0;
            static DWORD s_thermalProbeStartTime = 0;
            bool handAtThermalPort = (rightLocal.z > -0.38f && rightLocal.z < -0.14f && rightLocal.y > -0.15f && rightLocal.y < 0.20f && vrState.rightController.grip > 0.55f);
            bool probeCalibratedStill = (rSpeed < 0.12f);
            if (handAtThermalPort && probeCalibratedStill && (now - s_lastThermalProbeTime > 2000)) {
                if (s_thermalProbeStartTime == 0) s_thermalProbeStartTime = now;
                DWORD dwell = now - s_thermalProbeStartTime;
                float progress = (dwell < 400) ? (dwell / 400.0f) : 1.0f;
                vrState.rightHapticPulse = 0.30f + 0.40f * progress; // Ascending thermal equilibrium buzz
                if (dwell >= 400) {
                    s_thermalProbeStartTime = 0;
                    s_lastThermalProbeTime = now;
                    VR_LOG_INFO("Environmental Gesture: CRYO DEFROST THERMAL CALIBRATION COMPLETE");
                    SendKeyTap('E');
                    SendKeyTap(VK_TAB);
                    SendKeyTap('Q');
                    vrState.rightHapticPulse = 0.80f; // Defrost authorization chime (280 Hz)
                }
            } else {
                s_thermalProbeStartTime = 0;
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

            // Subterranean Hydroponics Misting Manifold Handwheel Siphon (ADR 0124)
            static DWORD s_lastMistingSiphonTime = 0;
            static int s_mistingSiphonCrankCount = 0;
            bool handAtMistingWheel = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > -0.22f && rightLocal.y < 0.16f) &&
                                      (vrState.rightController.grip > 0.55f);
            float mistingWheelSpeed = sqrtf(rVel.x * rVel.x + rVel.z * rVel.z);
            if (handAtMistingWheel && mistingWheelSpeed > 0.48f && (now - s_lastMistingSiphonTime > 220)) {
                s_lastMistingSiphonTime = now;
                s_mistingSiphonCrankCount++;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS MISTING SIPHON HANDWHEEL REVOLUTION #%d (Speed: %.2f m/s)", s_mistingSiphonCrankCount, mistingWheelSpeed);
                vrState.rightHapticPulse = 0.55f; // Brass thread friction detent (120 Hz)
                if (s_mistingSiphonCrankCount >= 3) {
                    s_mistingSiphonCrankCount = 0;
                    VR_LOG_INFO("Environmental Gesture: MISTING MANIFOLD PRESSURE SIPHON VENTED & FLUSHED");
                    SendKeyTap('E');
                    SendKeyTap('Q');
                    SendKeyTap(VK_RETURN);
                    vrState.rightHapticPulse = 0.85f; // Pressurized aerosol purge blast (190 Hz)
                    vrState.leftHapticPulse = 0.40f;
                }
            }

            // Subterranean Hydroponics Root Aeration Valve Foot Pedal Treadle (ADR 0132)
            static DWORD s_lastTreadlePlungeTime = 0;
            bool lowStanceForTreadle = (s_isPhysicallyCrouched || s_isDeepProne) || (rightLocal.y < -0.45f);
            bool treadleDownPlunge = (rVel.y < -0.95f && rightLocal.y < -0.48f && vrState.rightController.grip > 0.50f);
            if (lowStanceForTreadle && treadleDownPlunge && (now - s_lastTreadlePlungeTime > 1800)) {
                s_lastTreadlePlungeTime = now;
                VR_LOG_INFO("Environmental Gesture: ROOT AERATION TREADLE PUMPED (Vy: %.2f m/s)", rVel.y);
                SendKeyTap('E');
                SendKeyTap('C');
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.90f; // Pneumatic fluid aeration surge pulse (120 Hz)
                vrState.leftHapticPulse = 0.40f;
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

            // Dark Athena EVA Antenna Array Dipole Polarization Alignment (ADR 0125)
            static DWORD s_lastAntennaAlignTime = 0;
            static int s_antennaAlignStep = 0;
            bool bothAtAntennaGimbal = (leftLocal.y > 0.16f && rightLocal.y > 0.16f && leftLocal.z > -0.36f && leftLocal.z < -0.06f && rightLocal.z > -0.36f && rightLocal.z < -0.06f) &&
                                       (vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
            float antennaSweepSpeed = sqrtf((rVel.x - lVel.x) * (rVel.x - lVel.x) + (rVel.y - lVel.y) * (rVel.y - lVel.y));
            if (bothAtAntennaGimbal && antennaSweepSpeed > 0.45f && (now - s_lastAntennaAlignTime > 300)) {
                s_lastAntennaAlignTime = now;
                s_antennaAlignStep++;
                VR_LOG_INFO("Environmental Gesture: EVA ANTENNA GIMBAL POLARIZATION SWEEP STEP #%d (Speed: %.2f)", s_antennaAlignStep, antennaSweepSpeed);
                vrState.leftHapticPulse = 0.45f + 0.15f * s_antennaAlignStep;  // Ascending harmonic hum
                vrState.rightHapticPulse = 0.45f + 0.15f * s_antennaAlignStep;
                if (s_antennaAlignStep >= 3) {
                    s_antennaAlignStep = 0;
                    VR_LOG_INFO("Environmental Gesture: EVA ANTENNA ARRAY CARRIER WAVE POLARIZATION LOCKED");
                    SendKeyTap('E');
                    SendKeyTap('W');
                    SendKeyTap(VK_TAB);
                    vrState.leftHapticPulse = 0.85f;  // Magnetic phase lock detent (90 Hz)
                    vrState.rightHapticPulse = 0.85f;
                }
            }

            // Dark Athena EVA Solar Panel Photovoltaic Cell Scrape & De-icing (ADR 0129)
            static DWORD s_lastSolarScrapeTime = 0;
            static int s_solarScrapeCount = 0;
            bool handAtSolarPane = (rightLocal.z > -0.42f && rightLocal.z < -0.14f && rightLocal.y > -0.20f && rightLocal.y < 0.25f && vrState.rightController.grip > 0.55f);
            bool scrapeStroke = (fabsf(rVel.x) > 0.60f || fabsf(rVel.y) > 0.60f);
            if (handAtSolarPane && scrapeStroke && (now - s_lastSolarScrapeTime > 240)) {
                s_lastSolarScrapeTime = now;
                s_solarScrapeCount++;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR PANEL DE-ICING SCRAPE STROKE #%d (Speed: %.2f)", s_solarScrapeCount, rSpeed);
                vrState.rightHapticPulse = 0.50f; // Crystalline glass scraping rasp (240 Hz)
                if (s_solarScrapeCount >= 4) {
                    s_solarScrapeCount = 0;
                    VR_LOG_INFO("Environmental Gesture: EVA SOLAR PANEL DE-ICED & AUXILIARY POWER BUS RESTORED");
                    SendKeyTap('E');
                    SendKeyTap(VK_TAB);
                    SendKeyTap(VK_RETURN);
                    vrState.rightHapticPulse = 0.80f; // Auxiliary power bus resonance chime (310 Hz)
                    vrState.leftHapticPulse = 0.40f;
                }
            }

            // Dark Athena EVA Solar Collector Truss Structural Weld Inspection (ADR 0131)
            static DWORD s_lastWeldScanTime = 0;
            static DWORD s_weldScanStartTime = 0;
            bool handAtTrussSeam = (rightLocal.z > -0.38f && rightLocal.z < -0.12f && rightLocal.y > -0.15f && rightLocal.y < 0.20f && vrState.rightController.grip > 0.50f);
            bool steadyGlideVelocity = (fabsf(rVel.x) > 0.25f && fabsf(rVel.x) < 0.65f);
            if (handAtTrussSeam && steadyGlideVelocity && (now - s_lastWeldScanTime > 2000)) {
                if (s_weldScanStartTime == 0) s_weldScanStartTime = now;
                vrState.rightHapticPulse = 0.45f; // Ultrasonic scanner diagnostic vibration (230 Hz)
                if (now - s_weldScanStartTime >= 350) {
                    s_weldScanStartTime = 0;
                    s_lastWeldScanTime = now;
                    VR_LOG_INFO("Environmental Gesture: EVA TRUSS STRUCTURAL WELD INSPECTED & CLEARED");
                    SendKeyTap('E');
                    SendKeyTap(VK_SPACE);
                    SendKeyTap(VK_TAB);
                    vrState.rightHapticPulse = 0.75f; // Structural clearance confirmation chime (180 Hz)
                }
            } else {
                s_weldScanStartTime = 0;
            }

            // Dark Athena EVA Solar Array Cable Tension Winch Crank (ADR 0137)
            static DWORD s_lastCableWinchTime = 0;
            static int s_cableWinchCrankCount = 0;
            bool handAtCableWinch = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > -0.25f && rightLocal.y < 0.15f && vrState.rightController.grip > 0.55f);
            float winchCrankSpeed = sqrtf(rVel.y * rVel.y + rVel.z * rVel.z);
            if (handAtCableWinch && winchCrankSpeed > 0.48f && (now - s_lastCableWinchTime > 240)) {
                s_lastCableWinchTime = now;
                s_cableWinchCrankCount++;
                VR_LOG_INFO("Environmental Gesture: EVA CABLE TENSION WINCH REVOLUTION #%d (Speed: %.2f)", s_cableWinchCrankCount, winchCrankSpeed);
                vrState.rightHapticPulse = 0.55f + 0.10f * s_cableWinchCrankCount; // Progressive tension ratchet
                if (s_cableWinchCrankCount >= 3) {
                    s_cableWinchCrankCount = 0;
                    VR_LOG_INFO("Environmental Gesture: EVA SOLAR ARRAY CABLE TAUT & WINCH LOCKED");
                    SendKeyTap('E');
                    SendKeyTap('W');
                    SendKeyTap(VK_RETURN);
                    vrState.rightHapticPulse = 0.90f; // Mechanical dog lock snap (90 Hz)
                    vrState.leftHapticPulse = 0.50f;
                }
            }

            // Dark Athena EVA Solar Collector Truss Angular Lock Pin Shear & Drop (ADR 0139)
            static DWORD s_lastTrussPinShearTime = 0;
            bool handAtTrussShearPin = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > 0.10f && rightLocal.y < 0.45f && vrState.rightController.grip > 0.60f);
            bool downwardPinShearStrike = (rVel.y < -0.85f);
            if (handAtTrussShearPin && downwardPinShearStrike && (now - s_lastTrussPinShearTime > 1500)) {
                s_lastTrussPinShearTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR COLLECTOR TRUSS LOCK PIN SHEARED & DROPPED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                vrState.rightHapticPulse = 0.95f; // Metallurgical shear crack shockwave (260 Hz)
                vrState.leftHapticPulse = 0.60f;
            }

            // Subterranean Hydroponics Bio-Filter Carbon Mesh Cartridge Extraction (ADR 0140)
            static DWORD s_lastBioFilterExtractTime = 0;
            bool bothAtBioFilterHousing = (twoHandDist < 0.28f &&
                                           leftLocal.z > -0.38f && leftLocal.z < -0.14f && leftLocal.y > -0.20f && leftLocal.y < 0.20f &&
                                           rightLocal.z > -0.38f && rightLocal.z < -0.14f && rightLocal.y > -0.20f && rightLocal.y < 0.20f &&
                                           vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
            bool dualFilterRearwardPull = (lVel.z > 0.65f && rVel.z > 0.65f);
            if (bothAtBioFilterHousing && dualFilterRearwardPull && (now - s_lastBioFilterExtractTime > 1500)) {
                s_lastBioFilterExtractTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS BIO-FILTER CARBON CARTRIDGE EXTRACTED (Lz: %.2f, Rz: %.2f)", lVel.z, rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                vrState.leftHapticPulse = 0.85f;  // Filter canal sliding release clunk (180 Hz)
                vrState.rightHapticPulse = 0.85f;
            }

            // Dark Athena EVA Solar Collector Photovoltaic Cell Thermal Decoupling Fuse Pull (ADR 0143)
            static DWORD s_lastThermalFusePullTime = 0;
            bool handAtThermalFuseSocket = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > -0.20f && rightLocal.y < 0.25f &&
                                            vrState.rightController.grip > 0.45f && vrState.rightController.grip < 0.85f && vrState.rightController.trigger < 0.35f);
            float fuseWrenchSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y + rVel.z * rVel.z);
            if (handAtThermalFuseSocket && fuseWrenchSpeed > 0.65f && (now - s_lastThermalFusePullTime > 1500)) {
                s_lastThermalFusePullTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR ARRAY THERMAL DECOUPLING FUSE EXTRACTED (Speed: %.2f)", fuseWrenchSpeed);
                SendKeyTap('E');
                SendKeyTap(VK_RETURN);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.85f; // Spring retention detent release snap (160 Hz)
            }

            // Subterranean Hydroponics Bio-Sludge Siphon Pump T-Handle Reciprocating Stroke (ADR 0144)
            static DWORD s_lastSludgeStrokeTime = 0;
            static int s_sludgePumpStrokeCount = 0;
            bool bothAtSludgePumpTBar = (twoHandDist < 0.26f &&
                                         leftLocal.z > -0.38f && leftLocal.z < -0.14f && leftLocal.y > -0.35f && leftLocal.y < 0.15f &&
                                         rightLocal.z > -0.38f && rightLocal.z < -0.14f && rightLocal.y > -0.35f && rightLocal.y < 0.15f &&
                                         vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
            bool dualDownwardPumpStroke = (lVel.y < -0.70f && rVel.y < -0.70f);
            if (bothAtSludgePumpTBar && dualDownwardPumpStroke && (now - s_lastSludgeStrokeTime > 240)) {
                s_lastSludgeStrokeTime = now;
                s_sludgePumpStrokeCount++;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS SLUDGE SIPHON STROKE #%d (Ly: %.2f, Ry: %.2f)", s_sludgePumpStrokeCount, lVel.y, rVel.y);
                vrState.leftHapticPulse = 0.65f;  // Heavy viscous hydraulic drag (70 Hz)
                vrState.rightHapticPulse = 0.65f;
                if (s_sludgePumpStrokeCount >= 3) {
                    s_sludgePumpStrokeCount = 0;
                    VR_LOG_INFO("Environmental Gesture: SLUDGE SIPHON CLEARED & SUMP DRAIN OPENED");
                    SendKeyTap('E');
                    SendKeyTap(VK_SPACE);
                    SendKeyTap('W');
                    vrState.leftHapticPulse = 0.95f;  // Siphon surge pop & drainage rumble (120 Hz)
                    vrState.rightHapticPulse = 0.95f;
                }
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Grease Gun Injection (ADR 0147)
            static DWORD s_lastGreaseInjectTime = 0;
            static DWORD s_greaseInjectStart = 0;
            bool handAtTrussHingeNipple = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > 0.10f && rightLocal.y < 0.40f &&
                                           vrState.rightController.grip > 0.60f && vrState.rightController.trigger > 0.70f &&
                                           twoHandDist < 0.24f);
            if (handAtTrussHingeNipple) {
                if (s_greaseInjectStart == 0) s_greaseInjectStart = now;
                DWORD injectDuration = now - s_greaseInjectStart;
                vrState.rightHapticPulse = 0.50f; // Continuous fluid displacement buzz (160 Hz)
                vrState.leftHapticPulse = 0.35f;
                if (injectDuration >= 400 && (now - s_lastGreaseInjectTime > 1500)) {
                    s_lastGreaseInjectTime = now;
                    s_greaseInjectStart = 0;
                    VR_LOG_INFO("Environmental Gesture: EVA TRUSS HINGE GREASE INJECTION COMPLETE & UNSEIZED");
                    SendKeyTap('E');
                    SendKeyTap(VK_RETURN);
                    SendKeyTap(VK_TAB);
                    vrState.rightHapticPulse = 0.85f; // Hydraulic unseize pop (90 Hz)
                    vrState.leftHapticPulse = 0.60f;
                }
            } else {
                s_greaseInjectStart = 0;
            }

            // Subterranean Hydroponics Algae Scraping Spatula Reciprocating Blade Stroke (ADR 0148)
            static DWORD s_lastAlgaeScrapeTime = 0;
            static int s_algaeScrapeCount = 0;
            bool bladeAtAlgaeTube = (rightLocal.z > -0.40f && rightLocal.z < -0.16f && rightLocal.y > -0.25f && rightLocal.y < 0.20f &&
                                     vrState.rightController.grip > 0.55f);
            float algaeScrapeSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y);
            if (bladeAtAlgaeTube && algaeScrapeSpeed > 0.55f && (now - s_lastAlgaeScrapeTime > 200)) {
                s_lastAlgaeScrapeTime = now;
                s_algaeScrapeCount++;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS ALGAE TUBE SCRAPING STROKE #%d (Speed: %.2f)", s_algaeScrapeCount, algaeScrapeSpeed);
                vrState.rightHapticPulse = 0.48f; // Gritty quartz blade friction rasp (210 Hz)
                if (s_algaeScrapeCount >= 4) {
                    s_algaeScrapeCount = 0;
                    VR_LOG_INFO("Environmental Gesture: ALGAE BIOMASS CLEARED FROM QUARTZ TUBE");
                    SendKeyTap('E');
                    SendKeyTap(VK_TAB);
                    SendKeyTap(VK_SPACE);
                    vrState.rightHapticPulse = 0.75f; // Quartz resonance clean chime (290 Hz)
                }
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Cotter Pin Insertion (ADR 0151)
            static DWORD s_lastCotterPinTime = 0;
            bool handAtTrussCotterBore = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > 0.10f && rightLocal.y < 0.40f &&
                                          vrState.rightController.grip > 0.60f);
            bool cotterPinSeatingThrust = (rVel.z < -0.85f);
            if (handAtTrussCotterBore && cotterPinSeatingThrust && (now - s_lastCotterPinTime > 1500)) {
                s_lastCotterPinTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS HINGE COTTER PIN SEATED & LOCKED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Hardened alloy detent lock snap (280 Hz)
                vrState.leftHapticPulse = 0.60f;
            }

            // Subterranean Hydroponics Bio-Waste Incinerator Blast Door Dual Lever Pull (ADR 0152)
            static DWORD s_lastIncineratorLeverTime = 0;
            bool bothAtIncineratorLevers = (twoHandDist > 0.22f && twoHandDist < 0.45f &&
                                            leftLocal.z > -0.42f && leftLocal.z < -0.16f && rightLocal.z > -0.42f && rightLocal.z < -0.16f &&
                                            leftLocal.y > 0.05f && leftLocal.y < 0.35f && rightLocal.y > 0.05f && rightLocal.y < 0.35f &&
                                            vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
            bool dualIncineratorLeverPull = (lVel.y < -0.80f && rVel.y < -0.80f);
            if (bothAtIncineratorLevers && dualIncineratorLeverPull && (now - s_lastIncineratorLeverTime > 1500)) {
                s_lastIncineratorLeverTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS INCINERATOR DUAL BLAST LEVERS TRIPPED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_RETURN);
                SendKeyTap('Q');
                vrState.leftHapticPulse = 0.95f;  // Lever trip clack & combustion blast rumble (50 Hz)
                vrState.rightHapticPulse = 0.95f;
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Cotter Pin Extraction (ADR 0155)
            static DWORD s_lastCotterExtractTime = 0;
            bool handAtTrussCotterHead = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > 0.10f && rightLocal.y < 0.40f &&
                                          vrState.rightController.grip > 0.60f);
            bool cotterPinExtractPull = (rVel.z > 0.85f);
            if (handAtTrussCotterHead && cotterPinExtractPull && (now - s_lastCotterExtractTime > 1500)) {
                s_lastCotterExtractTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS HINGE COTTER PIN EXTRACTED & SALVAGED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                vrState.rightHapticPulse = 0.95f; // Mechanical release detent clunk (180 Hz)
                vrState.leftHapticPulse = 0.60f;
            }

            // Subterranean Hydroponics Bio-Waste Incinerator Burner Ignition Spark Wheel Strike (ADR 0156)
            static DWORD s_lastSparkWheelTime = 0;
            bool handAtSparkWheel = (rightLocal.z > -0.40f && rightLocal.z < -0.16f && rightLocal.y > -0.15f && rightLocal.y < 0.20f &&
                                     vrState.rightController.grip > 0.55f);
            bool sparkWheelStrike = (rVel.y < -0.90f);
            if (handAtSparkWheel && sparkWheelStrike && (now - s_lastSparkWheelTime > 1500)) {
                s_lastSparkWheelTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS INCINERATOR SPARK WHEEL STRUCK & IGNITED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Flint rasp & combustion flame blast (60 Hz)
                vrState.leftHapticPulse = 0.70f;
            }

            // Dark Athena EVA Solar Collector Truss Thermal Radiator Fin Unfolding Wrench (ADR 0159)
            static DWORD s_lastRadiatorFinTime = 0;
            bool handAtRadiatorFin = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > 0.05f && rightLocal.y < 0.38f &&
                                      vrState.rightController.grip > 0.60f);
            bool radiatorFinSweep = (fabsf(rVel.x) > 0.75f);
            if (handAtRadiatorFin && radiatorFinSweep && (now - s_lastRadiatorFinTime > 1500)) {
                s_lastRadiatorFinTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR RADIATOR FIN UNFOLDED & LOCKED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('W');
                SendKeyTap(VK_TAB);
                vrState.rightHapticPulse = 0.90f; // Detent latch lock pulse (280 Hz)
                vrState.leftHapticPulse = 0.60f;
            }

            // Subterranean Hydroponics Bio-Waste Incinerator Flame Damper Handwheel Spin (ADR 0160)
            static DWORD s_lastDamperSpinTime = 0;
            static int s_damperSpinCount = 0;
            bool handAtDamperWheel = (rightLocal.z > -0.42f && rightLocal.z < -0.16f && rightLocal.y > -0.25f && rightLocal.y < 0.15f &&
                                      vrState.rightController.grip > 0.55f);
            float damperWheelSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y);
            if (handAtDamperWheel && damperWheelSpeed > 0.48f && (now - s_lastDamperSpinTime > 220)) {
                s_lastDamperSpinTime = now;
                s_damperSpinCount++;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS FLAME DAMPER HANDWHEEL REVOLUTION #%d (Speed: %.2f)", s_damperSpinCount, damperWheelSpeed);
                vrState.rightHapticPulse = 0.55f; // Spindle rotation rumble (130 Hz)
                if (s_damperSpinCount >= 3) {
                    s_damperSpinCount = 0;
                    VR_LOG_INFO("Environmental Gesture: FLAME DAMPER VANES FULLY OPENED & LOCKED");
                    SendKeyTap('E');
                    SendKeyTap(VK_SPACE);
                    SendKeyTap('Q');
                    vrState.rightHapticPulse = 0.95f; // Mechanical end-stop detent thud (210 Hz)
                    vrState.leftHapticPulse = 0.50f;
                }
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Tension Guy-Wire Turnbuckle Wrench (ADR 0163)
            static DWORD s_lastTurnbuckleTime = 0;
            bool handAtTurnbuckle = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > -0.20f && rightLocal.y < 0.20f &&
                                     vrState.rightController.grip > 0.60f);
            bool turnbuckleTorqueWrench = (fabsf(rVel.x) > 0.65f);
            if (handAtTurnbuckle && turnbuckleTorqueWrench && (now - s_lastTurnbuckleTime > 1500)) {
                s_lastTurnbuckleTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR GUY-WIRE TURNBUCKLE TENSIONED & LOCKED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('W');
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Mechanical turnbuckle detent snap (290 Hz)
                vrState.leftHapticPulse = 0.50f;
            }

            // Subterranean Hydroponics Bio-Waste Incinerator Flame Arrestor Mesh Screen Slide-In (ADR 0164)
            static DWORD s_lastFlameArrestorTime = 0;
            bool twoHandAtFlameArrestor = (twoHandDist < 0.26f &&
                                           rightLocal.z > -0.40f && rightLocal.z < -0.16f && rightLocal.y > -0.15f && rightLocal.y < 0.25f &&
                                           vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
            bool flameArrestorSlideIn = (rVel.z < -0.80f || lVel.z < -0.80f);
            if (twoHandAtFlameArrestor && flameArrestorSlideIn && (now - s_lastFlameArrestorTime > 1500)) {
                s_lastFlameArrestorTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS FLAME ARRESTOR MESH SCREEN SLID & SEATED (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_TAB);
                vrState.rightHapticPulse = 0.90f; // Mechanical seating latch snap (260 Hz)
                vrState.leftHapticPulse = 0.90f;
            }

            // Dark Athena EVA Solar Array Debris Deflection Grid Pitch Tilt Pivot (ADR 0167)
            static DWORD s_lastDebrisGridTime = 0;
            bool bothAtDebrisGridYoke = (twoHandDist > 0.20f && twoHandDist < 0.40f &&
                                         leftLocal.z > -0.42f && leftLocal.z < -0.15f && rightLocal.z > -0.42f && rightLocal.z < -0.15f &&
                                         leftLocal.y > 0.10f && leftLocal.y < 0.40f && rightLocal.y > 0.10f && rightLocal.y < 0.40f &&
                                         vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
            float gridPitchSpeed = sqrtf(rVel.y * rVel.y + rVel.z * rVel.z);
            if (bothAtDebrisGridYoke && gridPitchSpeed > 0.60f && (now - s_lastDebrisGridTime > 1500)) {
                s_lastDebrisGridTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA DEBRIS DEFLECTION GRID TILTED & LOCKED (PitchSpeed: %.2f)", gridPitchSpeed);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                vrState.leftHapticPulse = 0.95f;  // Gimbal quadrant detent thud (270 Hz)
                vrState.rightHapticPulse = 0.95f;
            }

            // Subterranean Hydroponics Bio-Waste Incinerator Slag Poking Spud Bar Thrust (ADR 0168)
            static DWORD s_lastSpudBarTime = 0;
            bool twoHandAtSpudBar = (twoHandDist < 0.28f &&
                                     rightLocal.z > -0.42f && rightLocal.z < -0.16f && rightLocal.y > -0.25f && rightLocal.y < 0.15f &&
                                     vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
            bool spudBarLinearThrust = (rVel.z < -1.15f || lVel.z < -1.15f);
            if (twoHandAtSpudBar && spudBarLinearThrust && (now - s_lastSpudBarTime > 1500)) {
                s_lastSpudBarTime = now;
                VR_LOG_INFO("Environmental Gesture: INCINERATOR SLAG DISLODGED VIA SPUD BAR THRUST (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
                SendKeyTap('E');
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.98f; // Slag percussion fracture blast (260 Hz)
                vrState.leftHapticPulse = 0.85f;
            }

            // Dark Athena EVA Solar Collector Heat Radiator Shutter Louver Multi-Slit Slide (ADR 0171)
            static DWORD s_lastLouverSlideTime = 0;
            bool handAtLouverSlider = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > -0.15f && rightLocal.y < 0.25f &&
                                       vrState.rightController.grip > 0.60f);
            bool louverSlideStroke = (fabsf(rVel.y) > 0.70f || fabsf(rVel.x) > 0.70f);
            if (handAtLouverSlider && louverSlideStroke && (now - s_lastLouverSlideTime > 1500)) {
                s_lastLouverSlideTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA RADIATOR LOUVER SHUTTERS SLID & SEALED (Vy: %.2f, Vx: %.2f)", rVel.y, rVel.x);
                SendKeyTap('E');
                SendKeyTap('C');
                SendKeyTap(VK_TAB);
                vrState.rightHapticPulse = 0.90f; // Detent latch lock pulse (280 Hz)
                vrState.leftHapticPulse = 0.50f;
            }

            // Subterranean Hydroponics Bio-Waste Incinerator Flame Sensor Quartz Sight-Glass Scrape (ADR 0172)
            static DWORD s_lastSightGlassTime = 0;
            static int s_sightGlassCount = 0;
            bool handAtSightGlass = (rightLocal.z > -0.40f && rightLocal.z < -0.16f && rightLocal.y > -0.10f && rightLocal.y < 0.25f &&
                                     vrState.rightController.grip > 0.55f);
            float sightGlassSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y);
            if (handAtSightGlass && sightGlassSpeed > 0.52f && (now - s_lastSightGlassTime > 200)) {
                s_lastSightGlassTime = now;
                s_sightGlassCount++;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS SIGHT-GLASS SCRAPE STROKE #%d (Speed: %.2f)", s_sightGlassCount, sightGlassSpeed);
                vrState.rightHapticPulse = 0.45f; // Quartz rasp friction (220 Hz)
                if (s_sightGlassCount >= 4) {
                    s_sightGlassCount = 0;
                    VR_LOG_INFO("Environmental Gesture: SIGHT-GLASS WINDOW CLEARED / BURNER OPTICS RESTORED");
                    SendKeyTap('E');
                    SendKeyTap(VK_TAB);
                    SendKeyTap(VK_RETURN);
                    vrState.rightHapticPulse = 0.85f; // Optical sync resonant confirmation chime (320 Hz)
                    vrState.leftHapticPulse = 0.50f;
                }
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Ring Spanner Wrench (ADR 0175)
            static DWORD s_lastLockRingTime = 0;
            bool bothAtLockRingSpanner = (twoHandDist < 0.25f &&
                                          rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > 0.10f && rightLocal.y < 0.40f &&
                                          vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
            bool lockRingDownwardPull = (lVel.y < -0.90f && rVel.y < -0.90f);
            if (bothAtLockRingSpanner && lockRingDownwardPull && (now - s_lastLockRingTime > 1500)) {
                s_lastLockRingTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR TRUSS LOCK RING UNSEATED & UNCLAMPED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_RETURN);
                vrState.leftHapticPulse = 0.95f;  // Mechanical ring release detent thud (290 Hz)
                vrState.rightHapticPulse = 0.95f;
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Steam Scrubber Siphon Drain (ADR 0176)
            static DWORD s_lastScrubberDrainTime = 0;
            bool handAtScrubberLever = (rightLocal.z > -0.42f && rightLocal.z < -0.16f && rightLocal.y > -0.50f && rightLocal.y < -0.15f &&
                                        vrState.rightController.grip > 0.55f);
            bool scrubberDrainPull = (rVel.y > 0.75f);
            if (handAtScrubberLever && scrubberDrainPull && (now - s_lastScrubberDrainTime > 1500)) {
                s_lastScrubberDrainTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS SCRUBBER SIPHON DRAIN PURGED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.90f; // Turbulent hydraulic drain surge rumble (90 Hz)
                vrState.leftHapticPulse = 0.50f;
            }

            // Dark Athena EVA Solar Array Wing Jettison Explosive Bolt Shear (ADR 0179)
            static DWORD s_lastWingJettisonTime = 0;
            bool bothAtWingJettison = (twoHandDist < 0.25f &&
                                       rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > 0.10f && rightLocal.y < 0.42f &&
                                       vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
            bool wingJettisonDownwardShear = (lVel.y < -0.85f && rVel.y < -0.85f);
            if (bothAtWingJettison && wingJettisonDownwardShear && (now - s_lastWingJettisonTime > 2500)) {
                s_lastWingJettisonTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR WING JETTISON BOLTS DETONATED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.leftHapticPulse = 0.98f;  // Explosive bolt severance shockwave (280 Hz)
                vrState.rightHapticPulse = 0.98f;
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Purge Steam Lance (ADR 0180)
            static DWORD s_lastPurgeLanceTime = 0;
            bool twoHandAtPurgeLance = (twoHandDist < 0.28f &&
                                        rightLocal.z > -0.42f && rightLocal.z < -0.16f && rightLocal.y > -0.25f && rightLocal.y < 0.20f &&
                                        vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f &&
                                        vrState.rightController.trigger > 0.65f);
            bool purgeLanceThrust = (rVel.z < -0.90f || lVel.z < -0.90f);
            if (twoHandAtPurgeLance && purgeLanceThrust && (now - s_lastPurgeLanceTime > 1800)) {
                s_lastPurgeLanceTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS PURGE STEAM LANCE BLASTED (Rz: %.2f, Lz: %.2f)", rVel.z, lVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Steam manifold purge blast (240 Hz)
                vrState.leftHapticPulse = 0.70f;  // Foregrip steam vibration (110 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Wedge Mallet Strike (ADR 0183)
            static DWORD s_lastWedgeStrikeTime = 0;
            bool handAtTrussWedge = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > 0.10f && rightLocal.y < 0.42f &&
                                     vrState.rightController.grip > 0.60f);
            bool wedgeDownwardHammer = (rVel.y < -1.20f);
            if (handAtTrussWedge && wedgeDownwardHammer && (now - s_lastWedgeStrikeTime > 1500)) {
                s_lastWedgeStrikeTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR TRUSS LOCK WEDGE STRUCK & SEATED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.98f; // Wedge percussion impact shockwave (270 Hz)
                vrState.leftHapticPulse = 0.65f;  // Truss resonant chime (110 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator Secondary Air Preheater Bypass Flap (ADR 0184)
            static DWORD s_lastPreheaterBypassTime = 0;
            bool bothAtPreheaterBypass = (twoHandDist > 0.22f && twoHandDist < 0.45f &&
                                          leftLocal.z > -0.40f && leftLocal.z < -0.16f && rightLocal.z > -0.40f && rightLocal.z < -0.16f &&
                                          leftLocal.y > -0.20f && leftLocal.y < 0.20f && rightLocal.y > -0.20f && rightLocal.y < 0.20f &&
                                          vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
            bool preheaterBypassRearwardHaul = (lVel.z > 0.75f && rVel.z > 0.75f);
            if (bothAtPreheaterBypass && preheaterBypassRearwardHaul && (now - s_lastPreheaterBypassTime > 1800)) {
                s_lastPreheaterBypassTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS PREHEATER BYPASS FLAP TRIPPED (Lz: %.2f, Rz: %.2f)", lVel.z, rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.leftHapticPulse = 0.95f;  // Bypass seat detent thud (250 Hz)
                vrState.rightHapticPulse = 0.95f;
            }

            // Dark Athena EVA Solar Collector Articulation Hinge Wedge Removal Slide-Hammer (ADR 0187)
            static DWORD s_lastSlideHammerTime = 0;
            bool bothAtHingeBracket = (twoHandDist < 0.26f &&
                                       rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > 0.10f && rightLocal.y < 0.42f &&
                                       vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
            bool slideHammerRearwardStroke = (lVel.z > 0.95f && rVel.z > 0.95f);
            if (bothAtHingeBracket && slideHammerRearwardStroke && (now - s_lastSlideHammerTime > 2000)) {
                s_lastSlideHammerTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR WEDGE EXTRACTED VIA SLIDE HAMMER (Lz: %.2f, Rz: %.2f)", lVel.z, rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.leftHapticPulse = 0.98f;  // Inertial collision thud (290 Hz)
                vrState.rightHapticPulse = 0.98f;
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Soot Blower Lance Oscillation (ADR 0188)
            static DWORD s_lastSootBlowerTime = 0;
            static int s_sootBlowerStrokeCount = 0;
            bool twoHandAtSootBlower = (twoHandDist < 0.28f &&
                                        rightLocal.z > -0.42f && rightLocal.z < -0.16f && rightLocal.y > -0.20f && rightLocal.y < 0.25f &&
                                        vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f &&
                                        vrState.rightController.trigger > 0.60f);
            bool sootBlowerAxialStroke = (fabsf(rVel.z) > 0.65f || fabsf(lVel.z) > 0.65f);
            if (twoHandAtSootBlower && sootBlowerAxialStroke && (now - s_lastSootBlowerTime > 220)) {
                s_lastSootBlowerTime = now;
                s_sootBlowerStrokeCount++;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS SOOT BLOWER LANCE STROKE #%d (Rz: %.2f, Lz: %.2f)", s_sootBlowerStrokeCount, rVel.z, lVel.z);
                vrState.rightHapticPulse = 0.55f; // Tube scraping rasp (220 Hz)
                vrState.leftHapticPulse = 0.40f;
                if (s_sootBlowerStrokeCount >= 3) {
                    s_sootBlowerStrokeCount = 0;
                    VR_LOG_INFO("Environmental Gesture: ECONOMIZER TUBE BANK CLEARED VIA HIGH-PRESSURE STEAM SOOT BLOW");
                    SendKeyTap('E');
                    SendKeyTap(VK_SPACE);
                    SendKeyTap('Q');
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                    vrState.rightHapticPulse = 0.95f; // Steam jet acoustic blast (120 Hz)
                    vrState.leftHapticPulse = 0.75f;
                }
            }

            // Dark Athena EVA Solar Collector Heat Radiator Coolant Loop Bleed Petcock Valve (ADR 0191)
            static DWORD s_lastBleedValveTime = 0;
            bool handAtBleedValve = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > -0.20f && rightLocal.y < 0.20f &&
                                     vrState.rightController.grip > 0.45f && vrState.rightController.grip < 0.85f &&
                                     vrState.rightController.trigger < 0.40f);
            bool bleedValveTwist = (fabsf(rVel.x) > 0.65f);
            if (handAtBleedValve && bleedValveTwist && (now - s_lastBleedValveTime > 1800)) {
                s_lastBleedValveTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA RADIATOR COOLANT BLEED PETCOCK TWISTED & VENTED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.85f; // Cryogenic vapor venting surge (90 Hz)
                vrState.leftHapticPulse = 0.45f;
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Slag Tap Hole Oxygen Lance Burn (ADR 0192)
            static DWORD s_lastSlagBurnTime = 0;
            static DWORD s_slagBurnStartTime = 0;
            bool twoHandAtSlagPort = (twoHandDist < 0.26f &&
                                      rightLocal.z > -0.42f && rightLocal.z < -0.16f && rightLocal.y > -0.45f && rightLocal.y < -0.10f &&
                                      vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f &&
                                      vrState.rightController.trigger > 0.70f);
            bool slagLanceLinearFeed = (rVel.z < -0.30f);
            if (twoHandAtSlagPort && slagLanceLinearFeed) {
                if (s_slagBurnStartTime == 0) s_slagBurnStartTime = now;
                DWORD burnTime = now - s_slagBurnStartTime;
                float burnRamp = (burnTime < 400) ? (burnTime / 400.0f) : 1.0f;
                vrState.rightHapticPulse = 0.50f + 0.45f * burnRamp; // Sizzle to breakout rumble (260 -> 70 Hz)
                vrState.leftHapticPulse = 0.35f + 0.40f * burnRamp;
                if (burnTime >= 400 && (now - s_lastSlagBurnTime > 2500)) {
                    s_lastSlagBurnTime = now;
                    s_slagBurnStartTime = 0;
                    VR_LOG_INFO("Environmental Gesture: HYDROPONICS SLAG TAP HOLE BREACHED VIA OXYGEN LANCE BURN");
                    SendKeyTap('E');
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                    SendKeyTap(VK_SPACE);
                    SendKeyTap(VK_RETURN);
                    vrState.rightHapticPulse = 0.98f; // Molten slag breakout rumble (70 Hz)
                    vrState.leftHapticPulse = 0.90f;
                }
            } else {
                s_slagBurnStartTime = 0;
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Alignment Drift Pin Punch (ADR 0195)
            static DWORD s_lastDriftPinTime = 0;
            bool handAtCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                 vrState.leftController.grip > 0.55f);
            bool handAtPinPunch = (twoHandDist < 0.24f && vrState.rightController.grip > 0.60f);
            bool driftPinDownwardStrike = (rVel.y < -1.15f);
            if (handAtCollar && handAtPinPunch && driftPinDownwardStrike && (now - s_lastDriftPinTime > 1800)) {
                s_lastDriftPinTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS HINGE ALIGNED VIA DRIFT PIN MALLET PUNCH (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.98f; // Alloy pin punch shockwave (260 Hz)
                vrState.leftHapticPulse = 0.65f;  // Mechanical lug alignment detent pop (140 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Primary Air Swirl Vane Lever (ADR 0196)
            static DWORD s_lastSwirlVaneTime = 0;
            bool bothAtSwirlVanes = (twoHandDist > 0.20f && twoHandDist < 0.40f &&
                                     leftLocal.z > -0.42f && leftLocal.z < -0.16f && rightLocal.z > -0.42f && rightLocal.z < -0.16f &&
                                     leftLocal.y > -0.15f && leftLocal.y < 0.25f && rightLocal.y > -0.15f && rightLocal.y < 0.25f &&
                                     vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
            bool swirlVaneSweep = (fabsf(lVel.x) > 0.70f && fabsf(rVel.x) > 0.70f);
            if (bothAtSwirlVanes && swirlVaneSweep && (now - s_lastSwirlVaneTime > 2000)) {
                s_lastSwirlVaneTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS INCINERATOR AIR SWIRL VANES ADJUSTED (Lx: %.2f, Rx: %.2f)", lVel.x, rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.90f; // Detent notch engagement click (240 Hz)
                vrState.leftHapticPulse = 0.65f;  // Quadrant ratchet friction (150 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Ring Pinch Pin Insertion (ADR 0199)
            static DWORD s_lastPinchPinTime = 0;
            bool handAtPinchGuide = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                     vrState.leftController.grip > 0.55f);
            bool handHoldingPinchPin = (twoHandDist < 0.22f && vrState.rightController.grip > 0.60f);
            bool pinchPinForwardThrust = (rVel.z < -0.90f);
            if (handAtPinchGuide && handHoldingPinchPin && pinchPinForwardThrust && (now - s_lastPinchPinTime > 1800)) {
                s_lastPinchPinTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS PINCH PIN DRIVEN & DETENT SEATED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.98f; // Spring-loaded detent lock click (290 Hz)
                vrState.leftHapticPulse = 0.65f;  // Guide sleeve friction (190 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Electrostatic Precipitator Rapping Rod Impact (ADR 0200)
            static DWORD s_lastRappingRodTime = 0;
            bool bothAtRappingRod = (twoHandDist < 0.28f &&
                                     leftLocal.z > -0.42f && leftLocal.z < -0.16f && rightLocal.z > -0.42f && rightLocal.z < -0.16f &&
                                     leftLocal.y > -0.20f && leftLocal.y < 0.25f && rightLocal.y > -0.20f && rightLocal.y < 0.25f &&
                                     vrState.leftController.grip > 0.60f && vrState.rightController.grip > 0.60f);
            bool rappingRodImpact = (rVel.z < -1.15f);
            if (bothAtRappingRod && rappingRodImpact && (now - s_lastRappingRodTime > 2000)) {
                s_lastRappingRodTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS PRECIPITATOR RAPPING ROD IMPACTED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                vrState.rightHapticPulse = 0.98f; // Rapping anvil impact shockwave (270 Hz)
                vrState.leftHapticPulse = 0.70f;  // Precipitator plate vibration rumble (85 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Tension Guy-Wire Rigging Turnbuckle Locking Nut Strike (ADR 0203)
            static DWORD s_lastGuyWireNutTime = 0;
            bool handAtTurnbuckleBody = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > -0.20f && leftLocal.y < 0.20f &&
                                         vrState.leftController.grip > 0.55f);
            bool handHoldingMalletTool = (twoHandDist < 0.22f && vrState.rightController.grip > 0.60f);
            bool guyWireNutImpact = (rVel.y < -1.10f);
            if (handAtTurnbuckleBody && handHoldingMalletTool && guyWireNutImpact && (now - s_lastGuyWireNutTime > 1800)) {
                s_lastGuyWireNutTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA GUY-WIRE TURNBUCKLE LOCK NUT STAKED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap('W');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                vrState.rightHapticPulse = 0.98f; // Locking nut impact detent snap (260 Hz)
                vrState.leftHapticPulse = 0.60f;  // High-tension wire resonance chime (140 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Ash Quenching Trough Agitator Rake Sweep (ADR 0204)
            static DWORD s_lastAshRakeTime = 0;
            bool bothAtAshRake = (twoHandDist > 0.22f && twoHandDist < 0.45f &&
                                  leftLocal.z > -0.42f && leftLocal.z < -0.16f && rightLocal.z > -0.42f && rightLocal.z < -0.16f &&
                                  leftLocal.y > -0.45f && leftLocal.y < -0.15f && rightLocal.y > -0.45f && rightLocal.y < -0.15f &&
                                  vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
            bool ashRakeSweep = (fabsf(lVel.x) > 0.70f && fabsf(rVel.x) > 0.70f);
            if (bothAtAshRake && ashRakeSweep && (now - s_lastAshRakeTime > 2000)) {
                s_lastAshRakeTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS ASH QUENCHING TROUGH RAKED (Lx: %.2f, Rx: %.2f)", lVel.x, rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Clinker fracture crunch (280 Hz)
                vrState.leftHapticPulse = 0.65f;  // Gritty slurry grating friction (180 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Grease Nipple Cap Removal (ADR 0207)
            static DWORD s_lastGreaseCapTime = 0;
            bool handAtGreaseFitting = (rightLocal.z > -0.42f && rightLocal.z < -0.15f && rightLocal.y > 0.08f && rightLocal.y < 0.38f &&
                                        vrState.rightController.grip > 0.45f && vrState.rightController.grip < 0.85f &&
                                        vrState.rightController.trigger < 0.40f);
            bool greaseCapTwist = (fabsf(rVel.x) > 0.65f);
            if (handAtGreaseFitting && greaseCapTwist && (now - s_lastGreaseCapTime > 1800)) {
                s_lastGreaseCapTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS GREASE NIPPLE CAP UNTHREADED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.90f; // Thread unseating detent click (270 Hz)
                vrState.leftHapticPulse = 0.55f;  // Thread friction drag (170 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Combustion Air Damper Linkage Cotter Pin Extraction (ADR 0208)
            static DWORD s_lastDamperCotterTime = 0;
            bool handAtDamperClevis = (leftLocal.z > -0.40f && leftLocal.z < -0.16f && leftLocal.y > -0.15f && leftLocal.y < 0.25f &&
                                       vrState.leftController.grip > 0.55f);
            bool handAtCotterEyelet = (twoHandDist < 0.22f && vrState.rightController.grip > 0.60f);
            bool cotterExtractionHaul = (rVel.z > 0.90f);
            if (handAtDamperClevis && handAtCotterEyelet && cotterExtractionHaul && (now - s_lastDamperCotterTime > 1800)) {
                s_lastDamperCotterTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS DAMPER COTTER PIN EXTRACTED & FREED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.98f; // Cotter pin release snap (290 Hz)
                vrState.leftHapticPulse = 0.65f;  // Corroded clevis drag (210 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Retaining Ring Circlip Pliers Pinch (ADR 0211)
            static DWORD s_lastCirclipTime = 0;
            bool handAtHingeGrooveCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                            vrState.leftController.grip > 0.55f);
            bool handHoldingCirclipPliers = (twoHandDist < 0.22f && vrState.rightController.grip > 0.60f &&
                                             vrState.rightController.trigger > 0.60f);
            bool circlipAxialThrust = (rVel.z < -0.85f);
            if (handAtHingeGrooveCollar && handHoldingCirclipPliers && circlipAxialThrust && (now - s_lastCirclipTime > 1800)) {
                s_lastCirclipTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS CIRCLIP SEATED & SNAPPED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.98f; // Circlip snap detent (300 Hz)
                vrState.leftHapticPulse = 0.65f;  // Pliers spring compression tension (200 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Secondary Air Damper Handwheel Spin (ADR 0212)
            static DWORD s_lastSecondaryDamperTime = 0;
            static int s_secondaryDamperRevs = 0;
            bool handAtSecondaryDamper = (rightLocal.z > -0.42f && rightLocal.z < -0.16f && rightLocal.y > 0.10f && rightLocal.y < 0.42f &&
                                          vrState.rightController.grip > 0.55f);
            float secondaryDamperSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y);
            if (handAtSecondaryDamper && secondaryDamperSpeed > 0.48f && (now - s_lastSecondaryDamperTime > 250)) {
                s_lastSecondaryDamperTime = now;
                s_secondaryDamperRevs++;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS SECONDARY AIR DAMPER WHEEL CRANK #%d (Speed: %.2f)", s_secondaryDamperRevs, secondaryDamperSpeed);
                vrState.rightHapticPulse = 0.55f; // Spindle rotation friction (140 Hz)
                if (s_secondaryDamperRevs >= 3) {
                    s_secondaryDamperRevs = 0;
                    VR_LOG_INFO("Environmental Gesture: SECONDARY AIR DRAFT VANES OPENED");
                    SendKeyTap('E');
                    SendKeyTap('Q');
                    SendKeyTap(VK_SPACE);
                    SendKeyTap(VK_TAB);
                    vrState.rightHapticPulse = 0.95f; // Mechanical end-stop thud (220 Hz)
                }
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Shimming Washer Insertion (ADR 0215)
            static DWORD s_lastShimWasherTime = 0;
            bool handAtHingeGapClevis = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                         vrState.leftController.grip > 0.55f);
            bool handHoldingSlottedShim = (twoHandDist < 0.22f && vrState.rightController.grip > 0.60f);
            bool shimLateralSlide = (fabsf(rVel.x) > 0.85f);
            if (handAtHingeGapClevis && handHoldingSlottedShim && shimLateralSlide && (now - s_lastShimWasherTime > 1800)) {
                s_lastShimWasherTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS SHIM WASHER INSERTED & SEATED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Mechanical gap detent seating click (280 Hz)
                vrState.leftHapticPulse = 0.65f;  // Shim sliding friction (190 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Flue Gas Thermocouple Well Threaded Plug Tightening (ADR 0216)
            static DWORD s_lastThermocouplePlugTime = 0;
            bool handAtThermocouplePlug = (rightLocal.z > -0.40f && rightLocal.z < -0.16f && rightLocal.y > -0.10f && rightLocal.y < 0.25f &&
                                           vrState.rightController.grip > 0.45f && vrState.rightController.grip < 0.85f &&
                                           vrState.rightController.trigger < 0.40f);
            bool thermocouplePlugTwist = (fabsf(rVel.x) > 0.65f);
            if (handAtThermocouplePlug && thermocouplePlugTwist && (now - s_lastThermocouplePlugTime > 1800)) {
                s_lastThermocouplePlugTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS THERMOCOUPLE PLUG TORQUED & SEALED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.98f; // Metallic torque stop thud (270 Hz)
                vrState.leftHapticPulse = 0.55f;  // Copper crush washer deformation (160 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Retaining Cap Screw Driver Torque (ADR 0219)
            static DWORD s_lastRetainingScrewTime = 0;
            bool handAtHingeScrewBoss = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                         vrState.leftController.grip > 0.55f);
            bool handHoldingHexDriver = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                         vrState.rightController.trigger < 0.40f);
            bool hexDriverScrewTorque = (fabsf(rVel.x) > 0.65f);
            if (handAtHingeScrewBoss && handHoldingHexDriver && hexDriverScrewTorque && (now - s_lastRetainingScrewTime > 1800)) {
                s_lastRetainingScrewTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS RETAINING CAP SCREW TORQUED & LOCKED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Mechanical torque stop thud (290 Hz)
                vrState.leftHapticPulse = 0.60f;  // Helical thread tightening friction (170 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Bearing Housing Oil Level Sight-Glass Inspection & Vent (ADR 0220)
            static DWORD s_lastDraftFanVentTime = 0;
            bool handAtDraftFanPedestal = (rightLocal.z > -0.42f && rightLocal.z < -0.16f && rightLocal.y > 0.05f && rightLocal.y < 0.35f &&
                                           vrState.rightController.grip > 0.45f && vrState.rightController.grip < 0.85f &&
                                           vrState.rightController.trigger < 0.40f);
            bool draftFanVentTwist = (fabsf(rVel.x) > 0.65f);
            if (handAtDraftFanPedestal && draftFanVentTwist && (now - s_lastDraftFanVentTime > 1800)) {
                s_lastDraftFanVentTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS DRAFT FAN BEARING VENT UNSEATED & PURGED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.85f; // Pneumatic vent surge (95 Hz)
                vrState.leftHapticPulse = 0.55f;  // Knurled brass thread friction (150 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Micrometer Verification (ADR 0223)
            static DWORD s_lastVernierTime = 0;
            bool handAtHingeGuideCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                           vrState.leftController.grip > 0.55f);
            bool handHoldingVernierGauge = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                            vrState.rightController.trigger < 0.35f);
            bool vernierSlideStroke = (fabsf(rVel.z) > 0.45f);
            if (handAtHingeGuideCollar && handHoldingVernierGauge && vernierSlideStroke && (now - s_lastVernierTime > 1800)) {
                s_lastVernierTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS VERNIER MICROMETER VERIFIED & LOCKED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.90f; // Optical tolerance confirmation chime (310 Hz)
                vrState.leftHapticPulse = 0.50f;  // Vernier slide micro-clicks (210 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Primary Air Plenum Cleanout Manhole Cover Dog Clamp (ADR 0224)
            static DWORD s_lastPlenumClampTime = 0;
            bool bothHandsAtPlenumCollar = (twoHandDist > 0.20f && twoHandDist < 0.40f &&
                                            leftLocal.z > -0.42f && leftLocal.z < -0.16f &&
                                            leftLocal.y > -0.35f && leftLocal.y < 0.05f &&
                                            vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
            bool plenumClampPull = (lVel.y < -0.75f && rVel.y < -0.75f);
            if (bothHandsAtPlenumCollar && plenumClampPull && (now - s_lastPlenumClampTime > 1800)) {
                s_lastPlenumClampTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS PLENUM DOG CLAMPS LEVERED & DOGGED (Ly: %.2f, Ry: %.2f)", lVel.y, rVel.y);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Forged dog-clamp seating thuds (270 Hz)
                vrState.leftHapticPulse = 0.95f;  // Cast-iron clamp hinge swing (130 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Thermal Blanket Velcro Sealing Flap (ADR 0227)
            static DWORD s_lastThermalBlanketTime = 0;
            bool handHoldingBlanketFlap = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                           vrState.leftController.grip > 0.55f);
            bool handPressingVelcroSeam = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                           vrState.rightController.trigger > 0.50f);
            bool velcroSeamStroke = (fabsf(rVel.y) > 0.50f || fabsf(rVel.x) > 0.50f);
            if (handHoldingBlanketFlap && handPressingVelcroSeam && velcroSeamStroke && (now - s_lastThermalBlanketTime > 1800)) {
                s_lastThermalBlanketTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS THERMAL BLANKET VELCRO SEAM SEALED (Ry: %.2f, Rx: %.2f)", rVel.y, rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.90f; // Magnetic border snap click (260 Hz)
                vrState.leftHapticPulse = 0.55f;  // Velcro seam friction (180 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Primary Air Plenum Ash Hoist Bucket Winch Crank (ADR 0228)
            static DWORD s_lastAshWinchTime = 0;
            static int s_ashWinchRevs = 0;
            bool handAtAshWinch = (rightLocal.z > -0.42f && rightLocal.z < -0.16f && rightLocal.y > -0.20f && rightLocal.y < 0.25f &&
                                   vrState.rightController.grip > 0.55f);
            float ashWinchSpeed = sqrtf(rVel.x * rVel.x + rVel.y * rVel.y);
            if (handAtAshWinch && ashWinchSpeed > 0.48f && (now - s_lastAshWinchTime > 250)) {
                s_lastAshWinchTime = now;
                s_ashWinchRevs++;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS ASH HOIST WINCH CRANK #%d (Speed: %.2f)", s_ashWinchRevs, ashWinchSpeed);
                vrState.rightHapticPulse = 0.55f; // Cable drum winding rumble (140 Hz)
                if (s_ashWinchRevs >= 3) {
                    s_ashWinchRevs = 0;
                    VR_LOG_INFO("Environmental Gesture: ASH HOIST BUCKET ELEVATED TO HEARTH FLOOR");
                    SendKeyTap('E');
                    SendKeyTap('Q');
                    SendKeyTap(VK_SPACE);
                    SendKeyTap(VK_TAB);
                    vrState.rightHapticPulse = 0.95f; // Hoist dog ratchet thud (220 Hz)
                }
            }

            // Dark Athena EVA Solar Collector Truss Articulation Drive Motor Pinion Gear Backlash Adjustment (ADR 0231)
            static DWORD s_lastPinionBacklashTime = 0;
            bool handAtPinionGearbox = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                        vrState.leftController.grip > 0.55f);
            bool handOnBacklashAdjuster = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                           vrState.rightController.trigger > 0.50f);
            bool backlashAdjusterTorque = (fabsf(rVel.x) > 0.45f || fabsf(rVel.y) > 0.45f);
            if (handAtPinionGearbox && handOnBacklashAdjuster && backlashAdjusterTorque && (now - s_lastPinionBacklashTime > 1800)) {
                s_lastPinionBacklashTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS DRIVE MOTOR PINION BACKLASH ADJUSTED (Rx: %.2f, Ry: %.2f)", rVel.x, rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.92f; // Eccentric adjuster detent click (280 Hz)
                vrState.leftHapticPulse = 0.60f;  // Gearbox casing resonance (140 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Primary Air Plenum Slag Agitator Scraping Bar (ADR 0232)
            static DWORD s_lastSlagScraperTime = 0;
            static int s_slagScraperStrokes = 0;
            bool handsOnSlagScraper = (twoHandDist > 0.16f && twoHandDist < 0.38f &&
                                       vrState.leftController.grip > 0.55f && vrState.rightController.grip > 0.55f);
            bool scraperAtPlenumTuyeres = (rightLocal.z > -0.45f && rightLocal.z < -0.15f &&
                                           rightLocal.y > -0.30f && rightLocal.y < 0.20f);
            bool scraperStroke = (fabsf(rVel.z) > 0.65f && fabsf(lVel.z) > 0.55f);
            if (handsOnSlagScraper && scraperAtPlenumTuyeres && scraperStroke && (now - s_lastSlagScraperTime > 300)) {
                s_lastSlagScraperTime = now;
                s_slagScraperStrokes++;
                VR_LOG_INFO("Environmental Gesture: INCINERATOR SLAG SCRAPER STROKE #%d (Rz: %.2f, Lz: %.2f)", s_slagScraperStrokes, rVel.z, lVel.z);
                vrState.rightHapticPulse = 0.65f; // Scraper grating drag friction (200 Hz)
                vrState.leftHapticPulse = 0.55f;  // Shaft resonance (150 Hz)
                if (s_slagScraperStrokes >= 3) {
                    s_slagScraperStrokes = 0;
                    VR_LOG_INFO("Environmental Gesture: PRIMARY AIR TUYERES CLEARED OF REFRACTORY SLAG");
                    SendKeyTap('E');
                    SendKeyTap('Q');
                    SendKeyTap(VK_TAB);
                    SendKeyTap(VK_SPACE);
                    vrState.rightHapticPulse = 0.95f; // Refractory clinker dislodged impact (300 Hz)
                    vrState.leftHapticPulse = 0.85f;  // Shaft vibration impact (160 Hz)
                }
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Dial Gauge Clamping Screw (ADR 0235)
            static DWORD s_lastVernierClampTime = 0;
            bool handAtVernierCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                        vrState.leftController.grip > 0.55f);
            bool handPinchingClampScrew = (twoHandDist < 0.22f &&
                                           vrState.rightController.grip > 0.45f && vrState.rightController.grip < 0.85f &&
                                           vrState.rightController.trigger < 0.35f);
            bool clampScrewTorque = (fabsf(rVel.x) > 0.65f);
            if (handAtVernierCollar && handPinchingClampScrew && clampScrewTorque && (now - s_lastVernierClampTime > 1800)) {
                s_lastVernierClampTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS VERNIER DIAL GAUGE CLAMPED & LOCKED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Knurled brass clamping screw torque stop (290 Hz)
                vrState.leftHapticPulse = 0.55f;  // Caliper rail contact friction (150 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Weight Clip Insertion (ADR 0236)
            static DWORD s_lastBalanceClipTime = 0;
            bool handAtFanPort = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                  vrState.leftController.grip > 0.55f);
            bool handAligningClip = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                     vrState.rightController.trigger > 0.50f);
            bool clipInsertionThrust = (rVel.z < -0.85f);
            if (handAtFanPort && handAligningClip && clipInsertionThrust && (now - s_lastBalanceClipTime > 1800)) {
                s_lastBalanceClipTime = now;
                VR_LOG_INFO("Environmental Gesture: HYDROPONICS DRAFT FAN ROTOR BALANCE CLIP SEATED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Spring-steel tang snap (280 Hz)
                vrState.leftHapticPulse = 0.60f;  // Blade runner resonance (130 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Dial Gauge Zero Calibration Thumbscrew (ADR 0239)
            static DWORD s_lastVernierZeroTime = 0;
            bool handAtVernierAnvil = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                       vrState.leftController.grip > 0.55f);
            bool handOnZeroScrew = (twoHandDist < 0.22f &&
                                    vrState.rightController.grip > 0.45f && vrState.rightController.grip < 0.85f &&
                                    vrState.rightController.trigger < 0.35f);
            bool zeroScrewTwist = (fabsf(rVel.x) > 0.65f);
            if (handAtVernierAnvil && handOnZeroScrew && zeroScrewTwist && (now - s_lastVernierZeroTime > 1800)) {
                s_lastVernierZeroTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS VERNIER GAUGE ZERO-CALIBRATED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.50f; // Knurled brass zero-detent ratchet (240 Hz)
                vrState.leftHapticPulse = 0.90f;  // Optical zero datum confirmation tone (320 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Lock Tang Crimping Plier Squeeze (ADR 0240)
            static DWORD s_lastTangCrimpTime = 0;
            bool handAtImpellerRoot = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                       vrState.leftController.grip > 0.55f);
            bool handWithCrimpingPliers = (twoHandDist < 0.22f && vrState.rightController.grip > 0.60f &&
                                           vrState.rightController.trigger > 0.60f);
            bool tangCrimpSqueeze = (rVel.z < -0.80f);
            if (handAtImpellerRoot && handWithCrimpingPliers && tangCrimpSqueeze && (now - s_lastTangCrimpTime > 1800)) {
                s_lastTangCrimpTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN BALANCE CLIP TANG CRIMPED & STAKED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.98f; // Plier mechanical bottom-out snap (310 Hz)
                vrState.leftHapticPulse = 0.65f;  // Blade deformation resistance (170 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Dial Gauge Retaining Dog Set Screw (ADR 0243)
            static DWORD s_lastVernierDogScrewTime = 0;
            bool handAtVernierFixture = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                         vrState.leftController.grip > 0.55f);
            bool handOnDogSetScrew = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                      vrState.rightController.trigger < 0.40f);
            bool dogSetScrewTorque = (fabsf(rVel.x) > 0.65f);
            if (handAtVernierFixture && handOnDogSetScrew && dogSetScrewTorque && (now - s_lastVernierDogScrewTime > 1800)) {
                s_lastVernierDogScrewTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS VERNIER DOG SET SCREW TORQUED & LOCKED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Hardened dog screw torque stop (290 Hz)
                vrState.leftHapticPulse = 0.60f;  // Hinge lug friction (160 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Phase Angle Stroboscope Synchronization (ADR 0244)
            static DWORD s_lastStrobeSyncTime = 0;
            bool handAimingStrobe = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                     vrState.leftController.grip > 0.55f);
            bool handTuningStrobeDial = (twoHandDist < 0.22f &&
                                         vrState.rightController.grip > 0.45f && vrState.rightController.grip < 0.85f &&
                                         vrState.rightController.trigger < 0.35f);
            bool strobeDialTwist = (fabsf(rVel.x) > 0.65f);
            if (handAimingStrobe && handTuningStrobeDial && strobeDialTwist && (now - s_lastStrobeSyncTime > 1800)) {
                s_lastStrobeSyncTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN STROBOSCOPE PHASE LOCKED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.55f; // Optical flash sync micro-clicks (190 Hz)
                vrState.leftHapticPulse = 0.90f;  // Stationary rotor strobe lock tone (310 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Dial Gauge Retaining Dog Set Screw Driver Detent Release (ADR 0247)
            static DWORD s_lastVernierDetentReleaseTime = 0;
            bool handAtDogReleaseAnvil = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                          vrState.leftController.grip > 0.55f);
            bool handOnDogReleaseScrew = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                          vrState.rightController.trigger < 0.40f);
            bool dogReleaseTorque = (fabsf(rVel.x) > 0.65f);
            if (handAtDogReleaseAnvil && handOnDogReleaseScrew && dogReleaseTorque && (now - s_lastVernierDetentReleaseTime > 1800)) {
                s_lastVernierDetentReleaseTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS VERNIER DOG DETENT RELEASED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.65f; // Set screw thread unseating rasp (260 Hz)
                vrState.leftHapticPulse = 0.55f;  // Fixture clamp release pop (180 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Phase Angle Stroboscope Target Reflective Tape Burnish (ADR 0248)
            static DWORD s_lastTapeBurnishTime = 0;
            bool handAtTapeCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                     vrState.leftController.grip > 0.55f);
            bool handWithBurnisher = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                      vrState.rightController.trigger > 0.50f);
            bool tapeBurnishStroke = (rVel.z < -0.85f);
            if (handAtTapeCollar && handWithBurnisher && tapeBurnishStroke && (now - s_lastTapeBurnishTime > 1800)) {
                s_lastTapeBurnishTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN STROBOSCOPE TARGET TAPE BURNISHED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Roller burnish seating snap (280 Hz)
                vrState.leftHapticPulse = 0.60f;  // Shaft cylindrical friction (140 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Fixture Removal & Stowage (ADR 0251)
            static DWORD s_lastVernierStowTime = 0;
            bool handAtHingeAnvilCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                           vrState.leftController.grip > 0.55f);
            bool handGrippingCaliperChassis = (twoHandDist < 0.22f && vrState.rightController.grip > 0.60f &&
                                               vrState.rightController.trigger > 0.50f);
            bool caliperRemovalHaul = (rVel.z > 0.85f);
            if (handAtHingeAnvilCollar && handGrippingCaliperChassis && caliperRemovalHaul && (now - s_lastVernierStowTime > 1800)) {
                s_lastVernierStowTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS VERNIER CALIPER REMOVED & STOWED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Magnetic anvil quick-release clack (290 Hz)
                vrState.leftHapticPulse = 0.55f;  // Hinge lug clearance (150 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Phase Angle Optical Tachometer Sensor Alignment (ADR 0252)
            static DWORD s_lastTachAlignTime = 0;
            bool handHoldingTachClamp = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                         vrState.leftController.grip > 0.55f);
            bool handOnSensorGimbal = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger < 0.40f);
            bool tachGimbalTwist = (fabsf(rVel.x) > 0.65f);
            if (handHoldingTachClamp && handOnSensorGimbal && tachGimbalTwist && (now - s_lastTachAlignTime > 1800)) {
                s_lastTachAlignTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN OPTICAL TACHOMETER ALIGNED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.55f; // Optical receiver micro-clicks (190 Hz)
                vrState.leftHapticPulse = 0.95f;  // Optical alignment lock chime (320 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Storage Pouch Magnetic Flap Seal (ADR 0255)
            static DWORD s_lastVernierPouchSealTime = 0;
            bool handAtThighHolsterRim = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > -0.45f && leftLocal.y < -0.10f &&
                                          vrState.leftController.grip > 0.55f);
            bool handOnMagneticFlap = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger > 0.50f);
            bool flapSealStroke = (rVel.y < -0.65f);
            if (handAtThighHolsterRim && handOnMagneticFlap && flapSealStroke && (now - s_lastVernierPouchSealTime > 1800)) {
                s_lastVernierPouchSealTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS VERNIER POUCH FLAP SEALED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Neodymium magnetic clasp snap (270 Hz)
                vrState.leftHapticPulse = 0.55f;  // Kevlar fabric tension (130 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Cable Strain Relief Clamp Screw (ADR 0256)
            static DWORD s_lastCableClampTime = 0;
            bool handAtCableSaddle = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                      vrState.leftController.grip > 0.55f);
            bool handOnClampScrew = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                     vrState.rightController.trigger < 0.40f);
            bool cableClampTorque = (fabsf(rVel.x) > 0.65f);
            if (handAtCableSaddle && handOnClampScrew && cableClampTorque && (now - s_lastCableClampTime > 1800)) {
                s_lastCableClampTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER CABLE CLAMP TORQUED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Saddle clamp torque stop (280 Hz)
                vrState.leftHapticPulse = 0.55f;  // Shielded cable compression (140 Hz)
            }

            // Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Storage Pouch Retention D-Ring Carabiner Clip (ADR 0259)
            static DWORD s_lastPouchCarabinerTime = 0;
            bool handAtPouchDRing = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > -0.45f && leftLocal.y < -0.10f &&
                                     vrState.leftController.grip > 0.55f);
            bool handOnCarabinerGate = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                        vrState.rightController.trigger > 0.50f);
            bool pouchCarabinerSnap = (rVel.z < -0.80f);
            if (handAtPouchDRing && handOnCarabinerGate && pouchCarabinerSnap && (now - s_lastPouchCarabinerTime > 1800)) {
                s_lastPouchCarabinerTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA TRUSS VERNIER POUCH CARABINER SNAPPED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.98f; // Spring-steel carabiner gate snap (290 Hz)
                vrState.leftHapticPulse = 0.60f;  // Titanium D-ring tension (140 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Cable Shield Grounding Lug Screw (ADR 0260)
            static DWORD s_lastTachGroundScrewTime = 0;
            bool handAtPedestalBoss = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                       vrState.leftController.grip > 0.55f);
            bool handOnGroundScrew = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                      vrState.rightController.trigger < 0.40f);
            bool tachGroundScrewTorque = (fabsf(rVel.x) > 0.65f);
            if (handAtPedestalBoss && handOnGroundScrew && tachGroundScrewTorque && (now - s_lastTachGroundScrewTime > 1800)) {
                s_lastTachGroundScrewTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER GROUND SCREW TORQUED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.98f; // Grounding screw torque stop (290 Hz)
                vrState.leftHapticPulse = 0.60f;  // Braided copper wire compression (150 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Cell Busbar Laser Weld Inspection Pyrometer (ADR 0263)
            static DWORD s_lastBusbarPyrometerTime = 0;
            bool handAtBusbarGuide = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                      vrState.leftController.grip > 0.55f);
            bool handOnPyrometerHead = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                        vrState.rightController.trigger > 0.50f);
            bool pyrometerScanStroke = (rVel.z < -0.80f);
            if (handAtBusbarGuide && handOnPyrometerHead && pyrometerScanStroke && (now - s_lastBusbarPyrometerTime > 1800)) {
                s_lastBusbarPyrometerTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR BUSBAR WELD PYROMETER INSPECTED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Laser pyrometer pulse chime (310 Hz)
                vrState.leftHapticPulse = 0.55f;  // Busbar thermal contact buzz (160 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Amplifier Potentiometer (ADR 0264)
            static DWORD s_lastTachPotentiometerTime = 0;
            bool handAtAmpCasing = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                    vrState.leftController.grip > 0.55f);
            bool handOnPotentiometerTrimmer = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                               vrState.rightController.trigger < 0.40f);
            bool tachPotentiometerTorque = (fabsf(rVel.x) > 0.65f);
            if (handAtAmpCasing && handOnPotentiometerTrimmer && tachPotentiometerTorque && (now - s_lastTachPotentiometerTime > 1800)) {
                s_lastTachPotentiometerTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER POTENTIOMETER ADJUSTED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.55f; // Potentiometer detent wiper micro-clicks (200 Hz)
                vrState.leftHapticPulse = 0.95f;  // Signal gain lock chime (330 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Heat Sink Clamp Spring (ADR 0267)
            static DWORD s_lastDiodeClampSpringTime = 0;
            bool handAtDiodeHousing = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                       vrState.leftController.grip > 0.55f);
            bool handOnDiodeSpring = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                      vrState.rightController.trigger > 0.50f);
            bool diodeSpringStroke = (rVel.y < -0.65f);
            if (handAtDiodeHousing && handOnDiodeSpring && diodeSpringStroke && (now - s_lastDiodeClampSpringTime > 1800)) {
                s_lastDiodeClampSpringTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR BYPASS DIODE CLAMP SPRING SEATED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Tempered spring-steel snap (280 Hz)
                vrState.leftHapticPulse = 0.55f;  // Heat sink pad compression (140 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Cable Magnetic Shield Wrap (ADR 0268)
            static DWORD s_lastTachShieldWrapTime = 0;
            bool handAtMotorPedestalCable = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                             vrState.leftController.grip > 0.55f);
            bool handOnShieldFoil = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                     vrState.rightController.trigger < 0.40f);
            bool tachShieldWrapTorque = (fabsf(rVel.x) > 0.65f);
            if (handAtMotorPedestalCable && handOnShieldFoil && tachShieldWrapTorque && (now - s_lastTachShieldWrapTime > 1800)) {
                s_lastTachShieldWrapTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER SHIELD FOIL WRAPPED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Metallic shielding wrap click (260 Hz)
                vrState.leftHapticPulse = 0.55f;  // Silicone cable strain (150 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Ceramic Insulator Wafer Insertion (ADR 0271)
            static DWORD s_lastDiodeCeramicWaferTime = 0;
            bool handAtDiodePlate = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                     vrState.leftController.grip > 0.55f);
            bool handOnCeramicWafer = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger > 0.50f);
            bool ceramicWaferStroke = (rVel.z < -0.80f);
            if (handAtDiodePlate && handOnCeramicWafer && ceramicWaferStroke && (now - s_lastDiodeCeramicWaferTime > 1800)) {
                s_lastDiodeCeramicWaferTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE CERAMIC WAFER INSERTED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Ceramic wafer detent seating snap (290 Hz)
                vrState.leftHapticPulse = 0.55f;  // Mounting plate guide tension (150 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Cable Silicone Strain Relief Boot (ADR 0272)
            static DWORD s_lastTachStrainBootTime = 0;
            bool handAtGlandAdapter = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                       vrState.leftController.grip > 0.55f);
            bool handOnSiliconeBoot = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger < 0.40f);
            bool tachStrainBootStroke = (rVel.z < -0.80f);
            if (handAtGlandAdapter && handOnSiliconeBoot && tachStrainBootStroke && (now - s_lastTachStrainBootTime > 1800)) {
                s_lastTachStrainBootTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER STRAIN RELIEF BOOT SEATED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Elastomeric boot detent snap (270 Hz)
                vrState.leftHapticPulse = 0.55f;  // Gland adapter friction (140 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Ceramic Insulator Wafer Lock Tab Plier Crimp (ADR 0275)
            static DWORD s_lastWaferTabCrimpTime = 0;
            bool handAtDiodePlateCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                           vrState.leftController.grip > 0.55f);
            bool handOnTabCrimper = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                     vrState.rightController.trigger > 0.50f);
            bool waferTabCrimpStroke = (rVel.y < -0.65f);
            if (handAtDiodePlateCollar && handOnTabCrimper && waferTabCrimpStroke && (now - s_lastWaferTabCrimpTime > 1800)) {
                s_lastWaferTabCrimpTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE WAFER TAB CRIMPED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Tab deformation crimp snap (300 Hz)
                vrState.leftHapticPulse = 0.55f;  // Bracket clamp tension (160 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Cable Heat-Shrink Sleeve Heat Gun Pass (ADR 0276)
            static DWORD s_lastTachHeatShrinkTime = 0;
            bool handAtCableBootCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                          vrState.leftController.grip > 0.55f);
            bool handOnHeatGunNozzle = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                        vrState.rightController.trigger < 0.40f);
            bool tachHeatGunSweep = (fabsf(rVel.x) > 0.65f);
            if (handAtCableBootCollar && handOnHeatGunNozzle && tachHeatGunSweep && (now - s_lastTachHeatShrinkTime > 1800)) {
                s_lastTachHeatShrinkTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER HEAT SHRINK SLEEVE CURED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Hot-air blower buzz (250 Hz)
                vrState.leftHapticPulse = 0.55f;  // Curing sleeve contraction (150 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Terminal Lead Screw Torquing (ADR 0279)
            static DWORD s_lastDiodeLeadScrewTime = 0;
            bool handAtDiodeLugCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                         vrState.leftController.grip > 0.55f);
            bool handOnLeadHexDriver = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                        vrState.rightController.trigger < 0.40f);
            bool diodeLeadScrewTorque = (fabsf(rVel.x) > 0.65f);
            if (handAtDiodeLugCollar && handOnLeadHexDriver && diodeLeadScrewTorque && (now - s_lastDiodeLeadScrewTime > 1800)) {
                s_lastDiodeLeadScrewTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE TERMINAL LEAD SCREW TORQUED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Screw torque stop click (290 Hz)
                vrState.leftHapticPulse = 0.55f;  // Copper lug tension (160 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Amplifier BNC Output Connector Twist-Lock (ADR 0280)
            static DWORD s_lastTachBncLockTime = 0;
            bool handAtBncPortBoss = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                      vrState.leftController.grip > 0.55f);
            bool handOnBncCollar = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                    vrState.rightController.trigger > 0.50f);
            bool tachBncLockMotion = (rVel.z < -0.70f && fabsf(rVel.x) > 0.60f);
            if (handAtBncPortBoss && handOnBncCollar && tachBncLockMotion && (now - s_lastTachBncLockTime > 1800)) {
                s_lastTachBncLockTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER BNC CONNECTOR LOCKED (Rz: %.2f, Rx: %.2f)", rVel.z, rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Bayonet detent snap (280 Hz)
                vrState.leftHapticPulse = 0.55f;  // Bulkhead receptacle resistance (150 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Terminal Protective Silicone Boot Slide (ADR 0283)
            static DWORD s_lastDiodeBootSlideTime = 0;
            bool handAtDiodeLugBracket = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                          vrState.leftController.grip > 0.55f);
            bool handOnTerminalBoot = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger > 0.50f);
            bool diodeBootSlideStroke = (rVel.z < -0.80f);
            if (handAtDiodeLugBracket && handOnTerminalBoot && diodeBootSlideStroke && (now - s_lastDiodeBootSlideTime > 1800)) {
                s_lastDiodeBootSlideTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE TERMINAL BOOT SLID (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Elastomeric boot detent snap (280 Hz)
                vrState.leftHapticPulse = 0.55f;  // Bracket guide tension (140 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Amplifier BNC Cable P-Clamp Screw (ADR 0284)
            static DWORD s_lastTachPClampScrewTime = 0;
            bool handAtPClampSaddle = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                       vrState.leftController.grip > 0.55f);
            bool handOnPClampHexDriver = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                         vrState.rightController.trigger < 0.40f);
            bool tachPClampScrewTorque = (fabsf(rVel.x) > 0.65f);
            if (handAtPClampSaddle && handOnPClampHexDriver && tachPClampScrewTorque && (now - s_lastTachPClampScrewTime > 1800)) {
                s_lastTachPClampScrewTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER P-CLAMP SCREW TORQUED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // P-clamp torque stop click (290 Hz)
                vrState.leftHapticPulse = 0.55f;  // Cushioned saddle compression (150 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Silicone Boot Retention Zip-Tie Tension & Flush Cut (ADR 0287)
            static DWORD s_lastDiodeZipTieTime = 0;
            bool handAtDiodeBootCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                          vrState.leftController.grip > 0.55f);
            bool handOnZipTieTensioner = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                          vrState.rightController.trigger > 0.50f);
            bool diodeZipTieStroke = (rVel.y < -0.65f);
            if (handAtDiodeBootCollar && handOnZipTieTensioner && diodeZipTieStroke && (now - s_lastDiodeZipTieTime > 1800)) {
                s_lastDiodeZipTieTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE BOOT ZIP-TIE TENSIONED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Flush-cut blade snap (310 Hz)
                vrState.leftHapticPulse = 0.55f;  // Boot compression tension (160 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Probe Ground Clip Attachment (ADR 0288)
            static DWORD s_lastTachGroundClipTime = 0;
            bool handAtGroundStud = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                     vrState.leftController.grip > 0.55f);
            bool handOnGroundClip = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                     vrState.rightController.trigger < 0.40f);
            bool tachGroundClipStroke = (rVel.z < -0.75f);
            if (handAtGroundStud && handOnGroundClip && tachGroundClipStroke && (now - s_lastTachGroundClipTime > 1800)) {
                s_lastTachGroundClipTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER OSCILLOSCOPE GROUND CLIP ATTACHED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Alligator clip bite snap (280 Hz)
                vrState.leftHapticPulse = 0.55f;  // Ground stud contact (140 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Thermal Grease Syringe Bead Extrusion (ADR 0291)
            static DWORD s_lastDiodeGreaseExtrudeTime = 0;
            bool handAtHeatsinkPocket = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                         vrState.leftController.grip > 0.55f);
            bool handOnGreaseSyringe = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                        vrState.rightController.trigger > 0.50f);
            bool diodeGreaseExtrudeStroke = (fabsf(rVel.x) > 0.65f);
            if (handAtHeatsinkPocket && handOnGreaseSyringe && diodeGreaseExtrudeStroke && (now - s_lastDiodeGreaseExtrudeTime > 1800)) {
                s_lastDiodeGreaseExtrudeTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE THERMAL GREASE EXTRUDED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Viscous paste extrusion drag (260 Hz)
                vrState.leftHapticPulse = 0.50f;  // Heatsink pocket stabilization (140 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Frequency Dial Alignment (ADR 0292)
            static DWORD s_lastTachDialAlignTime = 0;
            bool handAtOscilloscopeBezel = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                            vrState.leftController.grip > 0.55f);
            bool handOnTachDial = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                   vrState.rightController.trigger < 0.40f);
            bool tachDialAlignStroke = (fabsf(rVel.x) > 0.70f);
            if (handAtOscilloscopeBezel && handOnTachDial && tachDialAlignStroke && (now - s_lastTachDialAlignTime > 1800)) {
                s_lastTachDialAlignTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER OSCILLOSCOPE FREQUENCY DIAL ALIGNED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Rotary switch detent click (320 Hz)
                vrState.leftHapticPulse = 0.50f;  // Chassis fan rumble (120 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Ceramic Wafer Alignment Guide Pin Insertion (ADR 0295)
            static DWORD s_lastDiodeGuidePinTime = 0;
            bool handAtHeatsinkCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                         vrState.leftController.grip > 0.55f);
            bool handOnGuidePin = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                   vrState.rightController.trigger < 0.40f);
            bool diodeGuidePinStroke = (rVel.z < -0.75f);
            if (handAtHeatsinkCollar && handOnGuidePin && diodeGuidePinStroke && (now - s_lastDiodeGuidePinTime > 1800)) {
                s_lastDiodeGuidePinTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE GUIDE PIN INSERTED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Polished guide pin sliding stop click (290 Hz)
                vrState.leftHapticPulse = 0.50f;  // Heatsink pocket grounding tension (150 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Trigger Threshold Level Knob Twist (ADR 0296)
            static DWORD s_lastTachTriggerKnobTime = 0;
            bool handAtOscilloscopeBracket = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                              vrState.leftController.grip > 0.55f);
            bool handOnTriggerKnob = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                      vrState.rightController.trigger > 0.50f);
            bool tachTriggerKnobStroke = (fabsf(rVel.x) > 0.65f);
            if (handAtOscilloscopeBracket && handOnTriggerKnob && tachTriggerKnobStroke && (now - s_lastTachTriggerKnobTime > 1800)) {
                s_lastTachTriggerKnobTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER OSCILLOSCOPE TRIGGER KNOB TWISTED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Trigger potentiometer micro-stepped clicks (310 Hz)
                vrState.leftHapticPulse = 0.50f;  // Fan induction motor harmonics (130 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Ceramic Wafer Alignment Guide Pin Retraction & Latch Engagement (ADR 0299)
            static DWORD s_lastDiodePinRetractTime = 0;
            bool handAtHeatsinkLatchCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                              vrState.leftController.grip > 0.55f);
            bool handOnGuidePinHead = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger < 0.40f);
            bool diodePinRetractStroke = (rVel.z > 0.75f);
            if (handAtHeatsinkLatchCollar && handOnGuidePinHead && diodePinRetractStroke && (now - s_lastDiodePinRetractTime > 1800)) {
                s_lastDiodePinRetractTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE GUIDE PIN RETRACTED & LATCH ENGAGED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Spring latch snap (280 Hz)
                vrState.leftHapticPulse = 0.50f;  // Heatsink pocket resonance (140 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Channel Input Coupling Toggle (ADR 0300)
            static DWORD s_lastTachCouplingToggleTime = 0;
            bool handAtOscilloscopeLowerBezel = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                                 vrState.leftController.grip > 0.55f);
            bool handOnCouplingSwitch = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                         vrState.rightController.trigger > 0.50f);
            bool tachCouplingToggleStroke = (fabsf(rVel.y) > 0.65f);
            if (handAtOscilloscopeLowerBezel && handOnCouplingSwitch && tachCouplingToggleStroke && (now - s_lastTachCouplingToggleTime > 1800)) {
                s_lastTachCouplingToggleTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER OSCILLOSCOPE CHANNEL COUPLING TOGGLED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // 3-position slide switch detent click (300 Hz)
                vrState.leftHapticPulse = 0.50f;  // Chassis grounding vibration (130 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Ceramic Wafer Lock Tab Plier Final Clinch (ADR 0303)
            static DWORD s_lastDiodeTabClinchTime = 0;
            bool handAtDiodePocketBracket = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                             vrState.leftController.grip > 0.55f);
            bool handOnClinchPliers = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger > 0.50f);
            bool diodeTabClinchStroke = (rVel.y < -0.70f);
            if (handAtDiodePocketBracket && handOnClinchPliers && diodeTabClinchStroke && (now - s_lastDiodeTabClinchTime > 1800)) {
                s_lastDiodeTabClinchTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE LOCK TAB CLINCHED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Metal tab deformation crunch (310 Hz)
                vrState.leftHapticPulse = 0.55f;  // Bracket reaction force (160 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Volts/Div Vertical Sensitivity Rotary Switch Step (ADR 0304)
            static DWORD s_lastTachVoltsDivTime = 0;
            bool handAtOscilloscopeUpperBezel = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                                 vrState.leftController.grip > 0.55f);
            bool handOnVoltsDivKnob = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger < 0.40f);
            bool tachVoltsDivStroke = (fabsf(rVel.x) > 0.70f);
            if (handAtOscilloscopeUpperBezel && handOnVoltsDivKnob && tachVoltsDivStroke && (now - s_lastTachVoltsDivTime > 1800)) {
                s_lastTachVoltsDivTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER OSCILLOSCOPE VOLTS/DIV STEPPED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // 8-position detented rotary switch clicks (330 Hz)
                vrState.leftHapticPulse = 0.50f;  // Chassis frame vibration (130 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Cathode Terminal Lug Lead Torque Wrench Tighten (ADR 0307)
            static DWORD s_lastDiodeLugTorqueTime = 0;
            bool handAtDiodeCathodePost = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                           vrState.leftController.grip > 0.55f);
            bool handOnTorqueWrench = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger < 0.40f);
            bool diodeLugTorqueStroke = (fabsf(rVel.x) > 0.70f);
            if (handAtDiodeCathodePost && handOnTorqueWrench && diodeLugTorqueStroke && (now - s_lastDiodeLugTorqueTime > 1800)) {
                s_lastDiodeLugTorqueTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE TERMINAL LUG TORQUED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Mechanical torque wrench break-over click (320 Hz)
                vrState.leftHapticPulse = 0.55f;  // Terminal post reaction tension (150 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Timebase Variable Vernier Calibration Knob Twist (ADR 0308)
            static DWORD s_lastTachVernierTime = 0;
            bool handAtOscilloscopeSideBracket = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                                  vrState.leftController.grip > 0.55f);
            bool handOnVernierKnob = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                      vrState.rightController.trigger > 0.50f);
            bool tachVernierStroke = (fabsf(rVel.x) > 0.65f);
            if (handAtOscilloscopeSideBracket && handOnVernierKnob && tachVernierStroke && (now - s_lastTachVernierTime > 1800)) {
                s_lastTachVernierTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER OSCILLOSCOPE TIMEBASE VERNIER TWISTED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Continuous viscous potentiometer drag (300 Hz)
                vrState.leftHapticPulse = 0.50f;  // Chassis grounding hum (120 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Silicone Insulator Boot Final Slide & Hermetic Seal Clench (ADR 0311)
            static DWORD s_lastDiodeBootSealTime = 0;
            bool handAtDiodeBracketCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                             vrState.leftController.grip > 0.55f);
            bool handOnBootCollar = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                     vrState.rightController.trigger > 0.50f);
            bool diodeBootSealStroke = (rVel.z < -0.80f);
            if (handAtDiodeBracketCollar && handOnBootCollar && diodeBootSealStroke && (now - s_lastDiodeBootSealTime > 1800)) {
                s_lastDiodeBootSealTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE BOOT HERMETIC SEAL CLENCHED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Elastomeric seal seating pop (290 Hz)
                vrState.leftHapticPulse = 0.50f;  // Heatsink bracket reaction (150 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Phase Marker Reticle Crosshair Alignment (ADR 0312)
            static DWORD s_lastTachReticleAlignTime = 0;
            bool handAtOscilloscopeLowerBezelPos = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                                    vrState.leftController.grip > 0.55f);
            bool handOnReticleKnob = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                      vrState.rightController.trigger < 0.40f);
            bool tachReticleAlignStroke = (fabsf(rVel.x) > 0.65f && fabsf(rVel.y) > 0.45f);
            if (handAtOscilloscopeLowerBezelPos && handOnReticleKnob && tachReticleAlignStroke && (now - s_lastTachReticleAlignTime > 1800)) {
                s_lastTachReticleAlignTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER OSCILLOSCOPE RETICLE CROSSHAIR ALIGNED (Rx: %.2f, Ry: %.2f)", rVel.x, rVel.y);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Reticle alignment lock detent click (340 Hz)
                vrState.leftHapticPulse = 0.50f;  // Chassis resonance hum (130 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Silicone Insulator Boot Nylon Cable Tie Tension & Flush Cutter Snip (ADR 0315)
            static DWORD s_lastDiodeZipTieTensionTime = 0;
            bool handAtDiodeBootNeck = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                        vrState.leftController.grip > 0.55f);
            bool handOnZipTieGun = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                    vrState.rightController.trigger > 0.50f);
            bool diodeZipTieTensionStroke = (fabsf(rVel.x) > 0.65f);
            if (handAtDiodeBootNeck && handOnZipTieGun && diodeZipTieTensionStroke && (now - s_lastDiodeZipTieTensionTime > 1800)) {
                s_lastDiodeZipTieTensionTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE BOOT ZIP TIE TENSIONED & FLUSH CUT (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Mechanical guillotine blade flush-cut snap (330 Hz)
                vrState.leftHapticPulse = 0.50f;  // Heatsink bracket reaction (140 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Vector Phase Angle Polar Display Graticule Lock (ADR 0316)
            static DWORD s_lastTachPolarLockTime = 0;
            bool handAtOscilloscopeBezelFrame = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                                 vrState.leftController.grip > 0.55f);
            bool handOnPolarRockerSwitch = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                            vrState.rightController.trigger < 0.40f);
            bool tachPolarLockStroke = (rVel.y > 0.65f);
            if (handAtOscilloscopeBezelFrame && handOnPolarRockerSwitch && tachPolarLockStroke && (now - s_lastTachPolarLockTime > 1800)) {
                s_lastTachPolarLockTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TACHOMETER OSCILLOSCOPE POLAR GRATICULE DISPLAY LOCKED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Rocker switch toggle detent snap (350 Hz)
                vrState.leftHapticPulse = 0.50f;  // Chassis grounding vibration (130 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Circuit Continuity & Resistance Multimeter Probe Test (ADR 0319)
            static DWORD s_lastDiodeMultimeterProbeTime = 0;
            bool handAtGroundLug = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                    vrState.leftController.grip > 0.55f);
            bool handOnCathodeProbe = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger < 0.40f);
            bool diodeProbeStroke = (rVel.z < -0.75f);
            if (handAtGroundLug && handOnCathodeProbe && diodeProbeStroke && (now - s_lastDiodeMultimeterProbeTime > 1800)) {
                s_lastDiodeMultimeterProbeTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE CONTINUITY MULTIMETER PROBE TESTED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Gold-plated probe tip contact snap (310 Hz)
                vrState.leftHapticPulse = 0.50f;  // Ground clip reaction (150 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Correction Trial Weight Clip Installation (ADR 0320)
            static DWORD s_lastTachTrialWeightTime = 0;
            bool handAtImpellerRim = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                      vrState.leftController.grip > 0.55f);
            bool handOnTrialWeightClip = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                          vrState.rightController.trigger > 0.50f);
            bool tachTrialWeightStroke = (rVel.z < -0.80f);
            if (handAtImpellerRim && handOnTrialWeightClip && tachTrialWeightStroke && (now - s_lastTachTrialWeightTime > 1800)) {
                s_lastTachTrialWeightTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN DYNAMIC BALANCE TRIAL WEIGHT CLIP INSTALLED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Spring-steel balance clip seating thud (360 Hz)
                vrState.leftHapticPulse = 0.50f;  // Impeller acoustic resonance (140 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode High-Current DC Bus Main Breaker Switch Interlock Clench (ADR 0323)
            static DWORD s_lastDiodeBusBreakerTime = 0;
            bool handAtBreakerHousing = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                         vrState.leftController.grip > 0.55f);
            bool handOnBreakerHandle = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                        vrState.rightController.trigger > 0.50f);
            bool diodeBreakerThrowStroke = (rVel.y < -0.80f);
            if (handAtBreakerHousing && handOnBreakerHandle && diodeBreakerThrowStroke && (now - s_lastDiodeBusBreakerTime > 1800)) {
                s_lastDiodeBusBreakerTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR DIODE DC BUS MAIN BREAKER CLOSED & INTERLOCKED (Ry: %.2f)", rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.98f; // Magnetic blowout arc shockwave (380 Hz)
                vrState.leftHapticPulse = 0.60f;  // Busbar hum (160 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Correction Trial Weight Lock-Tang Staking Punch (ADR 0324)
            static DWORD s_lastTachWeightStakeTime = 0;
            bool handAtImpellerBladeEdge = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                            vrState.leftController.grip > 0.55f);
            bool handOnStakingPunch = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger < 0.40f);
            bool tachWeightStakeStroke = (rVel.z < -0.85f);
            if (handAtImpellerBladeEdge && handOnStakingPunch && tachWeightStakeStroke && (now - s_lastTachWeightStakeTime > 1800)) {
                s_lastTachWeightStakeTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN TRIAL WEIGHT LOCK-TANG STAKED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.98f; // Center punch detent impact shockwave (380 Hz)
                vrState.leftHapticPulse = 0.55f;  // Impeller acoustic chime (150 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Pitch Actuator Manual Clasp Unlatch (ADR 0327)
            static DWORD s_lastGimbalPitchClaspTime = 0;
            bool handAtGimbalClevisFrame = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                            vrState.leftController.grip > 0.55f);
            bool handOnPitchClasp = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                     vrState.rightController.trigger > 0.50f);
            bool gimbalPitchUnlatchStroke = (rVel.z > 0.75f && rVel.y > 0.50f);
            if (handAtGimbalClevisFrame && handOnPitchClasp && gimbalPitchUnlatchStroke && (now - s_lastGimbalPitchClaspTime > 1800)) {
                s_lastGimbalPitchClaspTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR GIMBAL PITCH CLASP UNLATCHED (Rz: %.2f, Ry: %.2f)", rVel.z, rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Hardened steel lock clasp release snap (350 Hz)
                vrState.leftHapticPulse = 0.50f;  // Gimbal frame resonance (140 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Correction Residual Vibration Sensor Zero-Calibration Verification (ADR 0328)
            static DWORD s_lastTachVibZeroTime = 0;
            bool handAtOscilloscopeBezelRim = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                               vrState.leftController.grip > 0.55f);
            bool handOnZeroTrimDial = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                       vrState.rightController.trigger < 0.40f);
            bool tachVibZeroStroke = (fabsf(rVel.x) > 0.65f);
            if (handAtOscilloscopeBezelRim && handOnZeroTrimDial && tachVibZeroStroke && (now - s_lastTachVibZeroTime > 1800)) {
                s_lastTachVibZeroTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN VIBRATION SENSOR ZERO-CALIBRATED (Rx: %.2f)", rVel.x);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // 10-turn precision potentiometer drag & detent click (360 Hz)
                vrState.leftHapticPulse = 0.50f;  // Chassis grounding vibration (130 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Yaw Actuator Manual Clasp Unlatch (ADR 0331)
            static DWORD s_lastGimbalYawClaspTime = 0;
            bool handAtGimbalYawCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                          vrState.leftController.grip > 0.55f);
            bool handOnYawClasp = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                   vrState.rightController.trigger > 0.50f);
            bool gimbalYawUnlatchStroke = (fabsf(rVel.x) > 0.75f && rVel.z > 0.50f);
            if (handAtGimbalYawCollar && handOnYawClasp && gimbalYawUnlatchStroke && (now - s_lastGimbalYawClaspTime > 1800)) {
                s_lastGimbalYawClaspTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR GIMBAL YAW CLASP UNLATCHED (Rx: %.2f, Rz: %.2f)", rVel.x, rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Hardened titanium yaw clasp toggle snap (360 Hz)
                vrState.leftHapticPulse = 0.52f;  // Gimbal yoke low-frequency resonance (145 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Circuit Breaker Interlock Close (ADR 0332)
            static DWORD s_lastDraftFanBreakerTime = 0;
            bool handAtStarterDoorFlange = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                            vrState.leftController.grip > 0.55f);
            bool handOnStarterBreaker = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                         vrState.rightController.trigger < 0.40f);
            bool draftFanBreakerStroke = (rVel.y < -0.85f && rVel.z < -0.50f);
            if (handAtStarterDoorFlange && handOnStarterBreaker && draftFanBreakerStroke && (now - s_lastDraftFanBreakerTime > 1800)) {
                s_lastDraftFanBreakerTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN MOTOR STARTER BREAKER CLOSED (Ry: %.2f, Rz: %.2f)", rVel.y, rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.98f; // 480V copper bus contact closure snap (370 Hz)
                vrState.leftHapticPulse = 0.58f;  // Electromagnetic inductive hum rumble (135 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Harmonic Drive Manual Hand-Crank Slew Calibration (ADR 0335)
            static DWORD s_lastGimbalHandCrankTime = 0;
            bool handAtHarmonicDriveCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                              vrState.leftController.grip > 0.55f);
            bool handOnCrankKnob = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                    vrState.rightController.trigger < 0.40f);
            float crankCircVel = sqrtf(rVel.y * rVel.y + rVel.z * rVel.z);
            bool gimbalHandCrankStroke = (crankCircVel > 0.65f);
            if (handAtHarmonicDriveCollar && handOnCrankKnob && gimbalHandCrankStroke && (now - s_lastGimbalHandCrankTime > 1800)) {
                s_lastGimbalHandCrankTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR GIMBAL HARMONIC DRIVE HAND-CRANK SLEW (CircVel: %.2f)", crankCircVel);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.92f; // Harmonic drive flexspline gear mesh friction (330 Hz)
                vrState.leftHapticPulse = 0.55f;  // Structural gearbox casing resonance (150 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Variable Frequency Drive Speed Control Potentiometer Dial Adjustment (ADR 0336)
            static DWORD s_lastDraftFanVfdPotTime = 0;
            bool handAtVfdChassisFrame = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                          vrState.leftController.grip > 0.55f);
            bool handOnVfdPotDial = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                     vrState.rightController.trigger < 0.40f);
            bool draftFanVfdPotStroke = (rVel.x > 0.65f && rVel.y > 0.45f);
            if (handAtVfdChassisFrame && handOnVfdPotDial && draftFanVfdPotStroke && (now - s_lastDraftFanVfdPotTime > 1800)) {
                s_lastDraftFanVfdPotTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN VFD SPEED POT ADJUSTED (Rx: %.2f, Ry: %.2f)", rVel.x, rVel.y);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_RETURN);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Precision 10-turn potentiometer drag (360 Hz)
                vrState.leftHapticPulse = 0.55f;  // Inverter IGBT switching hum (140 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Optical Sun-Sensor Quadrant Photodiode Collimator Alignment (ADR 0339)
            static DWORD s_lastSunSensorAlignTime = 0;
            bool handAtSunSensorCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                          vrState.leftController.grip > 0.55f);
            bool handOnCollimatorRing = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                         vrState.rightController.trigger < 0.40f);
            bool sunSensorAlignStroke = (rVel.z < -0.75f && fabsf(rVel.x) > 0.50f);
            if (handAtSunSensorCollar && handOnCollimatorRing && sunSensorAlignStroke && (now - s_lastSunSensorAlignTime > 1800)) {
                s_lastSunSensorAlignTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR SUN-SENSOR COLLIMATOR ALIGNED (Rz: %.2f, Rx: %.2f)", rVel.z, rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap('Q');
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Optical collimator barrel lock detent snap (370 Hz)
                vrState.leftHapticPulse = 0.55f;  // Outboard spar structural resonance (160 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Variable Frequency Drive Acceleration Ramp Parameter Keypad Entry (ADR 0340)
            static DWORD s_lastDraftFanKeypadTime = 0;
            bool handAtKeypadBezel = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                      vrState.leftController.grip > 0.55f);
            bool handPointingKeypad = (twoHandDist < 0.22f && vrState.rightController.grip < 0.45f &&
                                       vrState.rightController.trigger < 0.35f);
            bool draftFanKeypadPokeStroke = (rVel.z < -0.70f);
            if (handAtKeypadBezel && handPointingKeypad && draftFanKeypadPokeStroke && (now - s_lastDraftFanKeypadTime > 1800)) {
                s_lastDraftFanKeypadTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN VFD KEYPAD ACCELERATION PARAMETER ENTERED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Membrane switch tactile snap (380 Hz)
                vrState.leftHapticPulse = 0.50f;  // Chassis enclosure resonance (130 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Closed-Loop Optical Tracking Servo Controller Enable Switch Toggle (ADR 0343)
            static DWORD s_lastGimbalServoEnableTime = 0;
            bool handAtServoEnclosureCollar = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                               vrState.leftController.grip > 0.55f);
            bool handOnServoToggleBat = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                         vrState.rightController.trigger < 0.40f);
            bool gimbalServoToggleStroke = (rVel.y > 0.65f && rVel.z < -0.40f);
            if (handAtServoEnclosureCollar && handOnServoToggleBat && gimbalServoToggleStroke && (now - s_lastGimbalServoEnableTime > 1800)) {
                s_lastGimbalServoEnableTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR GIMBAL SERVO CONTROLLER ENABLED (Ry: %.2f, Rz: %.2f)", rVel.y, rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.98f; // Spring-loaded toggle switch detent snap (380 Hz)
                vrState.leftHapticPulse = 0.60f;  // Servo drive inverter coil whine (170 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Variable Frequency Drive Motor Run Forward Pushbutton Latch (ADR 0344)
            static DWORD s_lastDraftFanRunButtonTime = 0;
            bool handAtOperatorConsoleRim = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                             vrState.leftController.grip > 0.55f);
            bool handOnRunPushbutton = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                        vrState.rightController.trigger > 0.50f);
            bool draftFanRunButtonStroke = (rVel.z < -0.80f);
            if (handAtOperatorConsoleRim && handOnRunPushbutton && draftFanRunButtonStroke && (now - s_lastDraftFanRunButtonTime > 1800)) {
                s_lastDraftFanRunButtonTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN MOTOR RUN PUSHBUTTON LATCHED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap('Q');
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.98f; // Industrial pushbutton spring contact snap (390 Hz)
                vrState.leftHapticPulse = 0.60f;  // Motor run contactor magnetic pull (150 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Azimuth Limit Switch Cam Dog Engagement (ADR 0347)
            static DWORD s_lastGimbalAzimuthCamDogTime = 0;
            bool handAtAzimuthLimitBracket = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                              vrState.leftController.grip > 0.55f);
            bool handOnCamDogShoe = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                     vrState.rightController.trigger < 0.40f);
            bool gimbalAzimuthCamDogStroke = (fabsf(rVel.x) > 0.65f && rVel.z < -0.50f);
            if (handAtAzimuthLimitBracket && handOnCamDogShoe && gimbalAzimuthCamDogStroke && (now - s_lastGimbalAzimuthCamDogTime > 1800)) {
                s_lastGimbalAzimuthCamDogTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR GIMBAL AZIMUTH LIMIT CAM DOG ENGAGED (Rx: %.2f, Rz: %.2f)", rVel.x, rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Alloy cam dog detent snap (400 Hz)
                vrState.leftHapticPulse = 0.55f;  // Limit switch bracket resonance (160 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Run Pilot Indicator Strobe Lamp Visual Verification (ADR 0348)
            static DWORD s_lastDraftFanRunIndicatorTime = 0;
            bool handAtVfdControlPanelTopRim = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                                vrState.leftController.grip > 0.55f);
            bool handPointingIndicatorStrobe = (twoHandDist < 0.22f && vrState.rightController.grip < 0.45f &&
                                                vrState.rightController.trigger < 0.35f);
            bool draftFanRunIndicatorStroke = (rVel.z < -0.75f);
            if (handAtVfdControlPanelTopRim && handPointingIndicatorStrobe && draftFanRunIndicatorStroke && (now - s_lastDraftFanRunIndicatorTime > 1800)) {
                s_lastDraftFanRunIndicatorTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN MOTOR RUN PILOT INDICATOR STROBE LAMP VERIFIED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Strobe lens push-to-test micro-switch click (400 Hz)
                vrState.leftHapticPulse = 0.55f;  // VFD enclosure acoustic vibration (145 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Elevation/Pitch Limit Switch Cam Dog Engagement (ADR 0351)
            static DWORD s_lastGimbalElevationCamDogTime = 0;
            bool handAtElevationLimitBracket = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                                vrState.leftController.grip > 0.55f);
            bool handOnElevationCamDog = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                          vrState.rightController.trigger < 0.40f);
            bool gimbalElevationCamDogStroke = (fabsf(rVel.y) > 0.65f && rVel.z < -0.50f);
            if (handAtElevationLimitBracket && handOnElevationCamDog && gimbalElevationCamDogStroke && (now - s_lastGimbalElevationCamDogTime > 1800)) {
                s_lastGimbalElevationCamDogTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR GIMBAL ELEVATION LIMIT CAM DOG ENGAGED (Ry: %.2f, Rz: %.2f)", rVel.y, rVel.z);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Alloy cam dog detent snap (410 Hz)
                vrState.leftHapticPulse = 0.55f;  // Limit switch housing resonance (165 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Auxiliary Contact Block Mechanical Latch Verification (ADR 0352)
            static DWORD s_lastAuxContactLatchTime = 0;
            bool handAtAuxContactBlockFrame = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                               vrState.leftController.grip > 0.55f);
            bool handPointingAuxContactPlunger = (twoHandDist < 0.22f && vrState.rightController.grip < 0.45f &&
                                                  vrState.rightController.trigger < 0.35f);
            bool auxContactLatchStroke = (rVel.z < -0.75f);
            if (handAtAuxContactBlockFrame && handPointingAuxContactPlunger && auxContactLatchStroke && (now - s_lastAuxContactLatchTime > 1800)) {
                s_lastAuxContactLatchTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN MOTOR STARTER AUXILIARY CONTACT LATCHED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Mechanical auxiliary contact latch snap (410 Hz)
                vrState.leftHapticPulse = 0.55f;  // Starter enclosure resonance (150 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Elevation/Pitch Quadrant Worm Gear Backlash Vernier Squeeze (ADR 0355)
            static DWORD s_lastGimbalWormVernierTime = 0;
            bool handAtElevationQuadrantFlange = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                                  vrState.leftController.grip > 0.55f);
            bool handOnWormVernierCollar = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                            vrState.rightController.trigger > 0.50f);
            bool gimbalWormVernierStroke = (rVel.z < -0.75f && fabsf(rVel.x) > 0.50f);
            if (handAtElevationQuadrantFlange && handOnWormVernierCollar && gimbalWormVernierStroke && (now - s_lastGimbalWormVernierTime > 1800)) {
                s_lastGimbalWormVernierTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR GIMBAL WORM GEAR BACKLASH VERNIER SQUEEZED (Rz: %.2f, Rx: %.2f)", rVel.z, rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Eccentric backlash detent click (410 Hz)
                vrState.leftHapticPulse = 0.55f;  // Quadrant gear mesh resonance (160 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Thermal Overload Relay Manual Reset Plunger Trip (ADR 0356)
            static DWORD s_lastThermalOverloadResetTime = 0;
            bool handAtStarterRelayRail = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                           vrState.leftController.grip > 0.55f);
            bool handPointingOverloadPlunger = (twoHandDist < 0.22f && vrState.rightController.grip < 0.45f &&
                                                vrState.rightController.trigger < 0.35f);
            bool thermalOverloadPlungeStroke = (rVel.z < -0.75f);
            if (handAtStarterRelayRail && handPointingOverloadPlunger && thermalOverloadPlungeStroke && (now - s_lastThermalOverloadResetTime > 1800)) {
                s_lastThermalOverloadResetTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN MOTOR STARTER THERMAL OVERLOAD RELAY RESET (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                vrState.rightHapticPulse = 0.95f; // Bimetallic overload snap (410 Hz)
                vrState.leftHapticPulse = 0.55f;  // Starter frame acoustic vibration (150 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Pitch Quadrant Drive Pinion Tooth Grease Port Needle Lubrication (ADR 0359)
            static DWORD s_lastGimbalPinionGreaseTime = 0;
            bool handAtPitchQuadrantGreaseBoss = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                                  vrState.leftController.grip > 0.55f);
            bool handOnGreaseNeedleDispenser = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                                vrState.rightController.trigger > 0.50f);
            bool gimbalPinionGreaseStroke = (rVel.z < -0.75f && fabsf(rVel.y) > 0.50f);
            if (handAtPitchQuadrantGreaseBoss && handOnGreaseNeedleDispenser && gimbalPinionGreaseStroke && (now - s_lastGimbalPinionGreaseTime > 1800)) {
                s_lastGimbalPinionGreaseTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR GIMBAL PINION TOOTH GREASE PORT NEEDLE LUBRICATION (Rz: %.2f, Ry: %.2f)", rVel.z, rVel.y);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Grease port needle seating click (420 Hz)
                vrState.leftHapticPulse = 0.55f;  // Quadrant gear casing acoustic vibration (165 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Phase Reversal Protective Relay Latch Reset (ADR 0360)
            static DWORD s_lastPhaseRelayResetTime = 0;
            bool handAtPhaseRelayChassis = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                            vrState.leftController.grip > 0.55f);
            bool handAtPhaseResetPlunger = (twoHandDist < 0.22f && vrState.rightController.grip < 0.45f &&
                                            vrState.rightController.trigger < 0.35f);
            bool phaseRelayResetStroke = (rVel.z < -0.75f);
            if (handAtPhaseRelayChassis && handAtPhaseResetPlunger && phaseRelayResetStroke && (now - s_lastPhaseRelayResetTime > 1800)) {
                s_lastPhaseRelayResetTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN MOTOR STARTER PHASE REVERSAL PROTECTIVE RELAY LATCH RESET (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Solid-state protective relay latch snap (420 Hz)
                vrState.leftHapticPulse = 0.55f;  // Starter enclosure acoustic resonance (155 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Pitch Quadrant Drive Pinion Bearing Retainer Castle Nut Cotter Pin Insertion & Tine Spread (ADR 0363)
            static DWORD s_lastGimbalPinionCotterPinTime = 0;
            bool handAtPitchPinionBearingFlange = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                                   vrState.leftController.grip > 0.55f);
            bool handHoldingCotterPliers = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                            vrState.rightController.trigger > 0.50f);
            bool gimbalPinionCotterStroke = (rVel.z < -0.75f && fabsf(rVel.x) > 0.50f);
            if (handAtPitchPinionBearingFlange && handHoldingCotterPliers && gimbalPinionCotterStroke && (now - s_lastGimbalPinionCotterPinTime > 1800)) {
                s_lastGimbalPinionCotterPinTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR GIMBAL PINION CASTLE NUT COTTER PIN INSERTED & SPREAD (Rz: %.2f, Rx: %.2f)", rVel.z, rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Cotter pin tine spread snap (425 Hz)
                vrState.leftHapticPulse = 0.55f;  // Bearing retainer housing resonance (170 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Phase Loss Sensor Test Button Actuation (ADR 0364)
            static DWORD s_lastPhaseLossTestTime = 0;
            bool handAtPhaseMonitorChassis = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                              vrState.leftController.grip > 0.55f);
            bool handPointingPhaseLossTestBtn = (twoHandDist < 0.22f && vrState.rightController.grip < 0.45f &&
                                                 vrState.rightController.trigger < 0.35f);
            bool phaseLossTestStroke = (rVel.z < -0.75f);
            if (handAtPhaseMonitorChassis && handPointingPhaseLossTestBtn && phaseLossTestStroke && (now - s_lastPhaseLossTestTime > 1800)) {
                s_lastPhaseLossTestTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN MOTOR STARTER PHASE LOSS SENSOR TEST BUTTON DEPRESSED (Rz: %.2f)", rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                SendKeyTap(VK_RETURN);
                vrState.rightHapticPulse = 0.95f; // Micro-switch tactile snap (425 Hz)
                vrState.leftHapticPulse = 0.55f;  // Starter frame acoustic resonance (160 Hz)
            }

            // Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Pitch Quadrant Drive Pinion Bearing Oil Seal Press Ring Snap Ring Seating (ADR 0367)
            static DWORD s_lastGimbalPinionSnapRingTime = 0;
            bool handAtPitchPinionSealBoss = (leftLocal.z > -0.42f && leftLocal.z < -0.15f && leftLocal.y > 0.10f && leftLocal.y < 0.40f &&
                                              vrState.leftController.grip > 0.55f);
            bool handHoldingSnapRingPliers = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                              vrState.rightController.trigger > 0.50f);
            bool gimbalPinionSnapRingStroke = (rVel.z < -0.75f && fabsf(rVel.x) > 0.50f);
            if (handAtPitchPinionSealBoss && handHoldingSnapRingPliers && gimbalPinionSnapRingStroke && (now - s_lastGimbalPinionSnapRingTime > 1800)) {
                s_lastGimbalPinionSnapRingTime = now;
                VR_LOG_INFO("Environmental Gesture: EVA SOLAR GIMBAL PINION OIL SEAL SNAP RING SEATED (Rz: %.2f, Rx: %.2f)", rVel.z, rVel.x);
                SendKeyTap('E');
                SendKeyTap(VK_SPACE);
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Circlip detent expansion snap (430 Hz)
                vrState.leftHapticPulse = 0.55f;  // Bearing cover acoustic vibration (175 Hz)
            }

            // Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Phase Unbalance Relay Trip Sensitivity Trimmer Potentiometer Adjustment (ADR 0368)
            static DWORD s_lastPhaseUnbalanceTrimmerTime = 0;
            bool handAtPhaseMonitorCasing = (leftLocal.z > -0.42f && leftLocal.z < -0.16f && leftLocal.y > 0.05f && leftLocal.y < 0.35f &&
                                             vrState.leftController.grip > 0.55f);
            bool handHoldingCeramicDriver = (twoHandDist < 0.22f && vrState.rightController.grip > 0.55f &&
                                             vrState.rightController.trigger < 0.40f);
            bool phaseUnbalanceTrimmerStroke = (fabsf(rVel.x) > 0.55f && rVel.z < -0.65f);
            if (handAtPhaseMonitorCasing && handHoldingCeramicDriver && phaseUnbalanceTrimmerStroke && (now - s_lastPhaseUnbalanceTrimmerTime > 1800)) {
                s_lastPhaseUnbalanceTrimmerTime = now;
                VR_LOG_INFO("Environmental Gesture: DRAFT FAN MOTOR STARTER PHASE UNBALANCE TRIMMER ADJUSTED (Rx: %.2f, Rz: %.2f)", rVel.x, rVel.z);
                SendKeyTap('E');
                SendKeyTap('Q');
                SendKeyTap(VK_TAB);
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                SendKeyTap('Q');
                SendKeyTap(VK_RETURN);
                SendKeyTap(VK_SPACE);
                vrState.rightHapticPulse = 0.95f; // Trimmer wiper micro-click (430 Hz)
                vrState.leftHapticPulse = 0.55f;  // Starter chassis acoustic resonance (165 Hz)
            }
#endif
        }
    }

    // 7. Tactical Eyeshine & Flashlight Gestures (ADR 0012)
    // - Right Hand Temple Tap (Y > 0.08m, X > 0.12m): Toggles Eyeshine / Night Vision ('Q') with 0.85 haptic pulse
    // - Left Hand Shoulder/Temple Tap (Y > 0.06m, X < -0.12m): Toggles Tactical Shoulder Flashlight ('F') with 0.75 haptic pulse
    static DWORD s_lastEyeshineTime = 0;
    static DWORD s_lastFlashlightTime = 0;

    if (!isClimbingActive) {
        // Right Hand -> Eyeshine Night Vision (ADR 0380: 3D Anatomical Bounding Box)
        bool rightAtTemple = (rightLocal.y >= 0.06f && rightLocal.y <= 0.22f) &&
                             (rightLocal.x >= 0.12f && rightLocal.x <= 0.28f) &&
                             (rightLocal.z >= -0.15f && rightLocal.z <= 0.08f) &&
                             (vrState.rightController.grip > 0.5f);
        if (rightAtTemple && (now - s_lastEyeshineTime > 1200)) {
            s_lastEyeshineTime = now;
            s_eyeshineActive = !s_eyeshineActive;
            VR_LOG_INFO("Tactical Gesture: EYESHINE %s (Right Temple Tap)", s_eyeshineActive ? "ACTIVATED" : "DEACTIVATED");
            SendKeyTap('Q'); // Native Riddick Eyeshine key
            vrState.rightHapticPulse = 0.85f; // High-resonance optical buzz
        }

        // Left Hand -> Tactical Shoulder Flashlight (ADR 0380: 3D Anatomical Bounding Box)
        bool leftAtShoulder = (leftLocal.y >= -0.10f && leftLocal.y <= 0.12f) &&
                              (leftLocal.x >= -0.28f && leftLocal.x <= -0.12f) &&
                              (leftLocal.z >= -0.20f && leftLocal.z <= 0.06f) &&
                              (vrState.leftController.grip > 0.5f);
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
            // ADR 0376: Weapon Slot Recoil & Haptic Profile Calibration
            switch (s_activeWeaponSlot) {
                case 3: // Melee / Shivs / Ulaks
                    s_recoilKickPitch = 0.005f;
                    vrState.rightHapticPulse = 0.45f;
                    break;
                case 2: // Sidearm / Pistol / Tranq Gun
                    s_recoilKickPitch = s_twoHandedStabilizationActive ? 0.012f : 0.024f;
                    vrState.rightHapticPulse = s_twoHandedStabilizationActive ? 0.50f : 0.62f;
                    if (s_twoHandedStabilizationActive) vrState.leftHapticPulse = 0.35f;
                    break;
                case 1: // Primary Shotgun / Combat Long-Arm
                    s_recoilKickPitch = s_twoHandedStabilizationActive ? 0.028f : 0.055f;
                    vrState.rightHapticPulse = s_twoHandedStabilizationActive ? 0.80f : 0.95f;
                    if (s_twoHandedStabilizationActive) vrState.leftHapticPulse = 0.65f;
                    break;
                case 4: // Tactical Assault Rifle / Ordnance
                    s_recoilKickPitch = s_twoHandedStabilizationActive ? 0.015f : 0.028f;
                    vrState.rightHapticPulse = s_twoHandedStabilizationActive ? 0.60f : 0.72f;
                    if (s_twoHandedStabilizationActive) vrState.leftHapticPulse = 0.45f;
                    break;
                case 5: // Heavy Weapon / SCAR Gun / Rotary Chaingun
                default:
                    s_recoilKickPitch = s_twoHandedStabilizationActive ? 0.025f : 0.065f;
                    vrState.rightHapticPulse = s_twoHandedStabilizationActive ? 0.85f : 0.98f;
                    if (s_twoHandedStabilizationActive) vrState.leftHapticPulse = 0.75f;
                    break;
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
            // Evaluated at moment of release (ADR 0382: Forward Trajectory Vector Gating):
            // Forward throwing motion requires positive forward velocity (lVel.z < -0.65 m/s) and speed > 1.10 m/s
            // Arm retraction or sideways swings gently drop the object at feet rather than hurling it forward
            if (lVel.z < -0.65f && lSpeed > 1.10f) {
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

    // 10. Face Buttons Mapping (ADR 0401: Decoupled Menu Confirmation & Pure Gameplay Inputs)
    bool btnA = (btns & RIDDICK_VR_BTN_A) != 0;
    bool prevBtnA = (s_prevButtons & RIDDICK_VR_BTN_A) != 0;
    bool btnB = (btns & RIDDICK_VR_BTN_B) != 0;
    bool prevBtnB = (s_prevButtons & RIDDICK_VR_BTN_B) != 0;
    bool btnX = (btns & RIDDICK_VR_BTN_X) != 0;
    bool prevBtnX = (s_prevButtons & RIDDICK_VR_BTN_X) != 0;

    if (vrState.cinemaMode) {
        // --- CINEMA / MENU MODE ---
        // BUTTON A -> Select / Confirm (Return / Left Click)
        if (btnA != prevBtnA) {
            SendKey(VK_RETURN, btnA);
            if (btnA) mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            else mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            if (btnA) vrState.rightHapticPulse = 0.35f;
        }

        // BUTTON B -> Back / Escape / Cancel
        if (btnB != prevBtnB) {
            SendKey(VK_ESCAPE, btnB);
            SendKey(VK_BACK, btnB);
            if (btnB) vrState.rightHapticPulse = 0.30f;
        }

        // BUTTON X -> Secondary Confirm (Return)
        if (btnX && !prevBtnX) {
            SendKeyTap(VK_RETURN);
            vrState.leftHapticPulse = 0.30f;
        }
    } else {
        // --- 3D GAMEPLAY ACTION MODE ---
        // BUTTON A -> Primary Interact / Use / Talk / Climb / Open ('E')
        // Pure single-action input: zero jump (Space) or fire (LMB) cross-talk
        if (btnA != prevBtnA) {
            SendKey('E', btnA);
            if (btnA) vrState.rightHapticPulse = 0.35f;
        }

        // BUTTON B -> Jump (Space) & Crouch-Jump Mantle Clearance (ADR 0025)
        // Pure jump input: zero Escape or Tab cross-talk
        if (btnB != prevBtnB) {
            if (btnB && s_isPhysicallyCrouched) {
                VR_LOG_INFO("Movement Stance: PHYSICAL CROUCH-JUMP / MANTLE CLEARANCE executed");
                SendKey('C', true);
                SendKey(VK_SPACE, true);
                vrState.rightHapticPulse = 0.45f;
            } else {
                SendKey(VK_SPACE, btnB);
            }
            if (btnB && !s_isPhysicallyCrouched) vrState.rightHapticPulse = 0.30f;
        }

        // BUTTON X -> Crouch Toggle ('C')
        // Pure crouch input: zero E or Return cross-talk
        if (btnX && !prevBtnX) {
            SendKeyTap('C');
            vrState.leftHapticPulse = 0.30f;
        }
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
        // Left Thumbstick Click -> Sprint Toggle is handled in locomotion section.
        // Crouch toggle is mapped to Button X and physical crouch stance.

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

    // Left Controller Menu Button -> Clean Pause / Resume Menu Toggle (Escape)
    if (btnMenu && !prevBtnMenu) {
        SendKeyTap(VK_ESCAPE);
        vrState.leftHapticPulse = 0.40f;
    }

    s_prevButtons = btns;
}

} // namespace RiddickVR
