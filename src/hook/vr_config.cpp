#include "vr_config.h"
#include "logging.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace RiddickVR {

VRConfig g_vrConfig{};

static float ClampF(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float ReadFloat(const char* iniPath, const char* key, float fallback) {
    char buf[64];
    if (GetPrivateProfileStringA("VR", key, "", buf, sizeof(buf), iniPath) == 0 || buf[0] == '\0')
        return fallback;
    return static_cast<float>(strtod(buf, nullptr));
}

static int ReadInt(const char* iniPath, const char* key, int fallback) {
    return static_cast<int>(GetPrivateProfileIntA("VR", key, fallback, iniPath));
}

// Track C #10: minimal INI loader. Search order: DLL directory, then CWD.
// Missing file or keys -> compiled defaults (build-safe, no external deps).
void LoadVRConfig() {
    VRConfig cfg; // defaults

    char iniPath[MAX_PATH] = {};
    bool haveIni = false;

    HMODULE hSelf = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(&LoadVRConfig), &hSelf) && hSelf) {
        char dllPath[MAX_PATH];
        if (GetModuleFileNameA(hSelf, dllPath, sizeof(dllPath))) {
            char* slash = strrchr(dllPath, '\\');
            if (slash) {
                *slash = '\0';
                _snprintf_s(iniPath, sizeof(iniPath), _TRUNCATE, "%s\\riddick_vr.ini", dllPath);
                DWORD attrs = GetFileAttributesA(iniPath);
                haveIni = (attrs != INVALID_FILE_ATTRIBUTES);
            }
        }
    }
    if (!haveIni) {
        _snprintf_s(iniPath, sizeof(iniPath), _TRUNCATE, "riddick_vr.ini");
        DWORD attrs = GetFileAttributesA(iniPath);
        haveIni = (attrs != INVALID_FILE_ATTRIBUTES);
    }

    if (haveIni) {
        cfg.snapAngleDeg = ClampF(ReadFloat(iniPath, "SnapAngleDeg", cfg.snapAngleDeg), 15.0f, 90.0f);
        cfg.smoothTurnSpeed = ClampF(ReadFloat(iniPath, "SmoothTurnSpeed", cfg.smoothTurnSpeed), 1.0f, 60.0f);
        cfg.moveDeadzone = ClampF(ReadFloat(iniPath, "MoveDeadzone", cfg.moveDeadzone), 0.05f, 0.60f);
        cfg.turnDeadzone = ClampF(ReadFloat(iniPath, "TurnDeadzone", cfg.turnDeadzone), 0.05f, 0.60f);
        cfg.dialogueDeadzone = ClampF(ReadFloat(iniPath, "DialogueDeadzone", cfg.dialogueDeadzone), 0.15f, 0.80f);
        cfg.interactButton = ReadInt(iniPath, "InteractButton", cfg.interactButton);
        if (cfg.interactButton < 0 || cfg.interactButton > 1) cfg.interactButton = 0;
        cfg.lookSensitivityYaw = ClampF(ReadFloat(iniPath, "LookSensitivityYaw", cfg.lookSensitivityYaw), 50.0f, 2000.0f);
        cfg.lookSensitivityPitch = ClampF(ReadFloat(iniPath, "LookSensitivityPitch", cfg.lookSensitivityPitch), 50.0f, 2000.0f);
        cfg.comfortEnabled = ReadInt(iniPath, "ComfortEnabled", cfg.comfortEnabled) ? 1 : 0;
        cfg.comfortStrength = ClampF(ReadFloat(iniPath, "ComfortStrength", cfg.comfortStrength), 0.0f, 2.0f);
        cfg.comfortMaxDim = ClampF(ReadFloat(iniPath, "ComfortMaxDim", cfg.comfortMaxDim), 0.0f, 1.0f);
        cfg.seated = ReadInt(iniPath, "Seated", cfg.seated) ? 1 : 0;
        cfg.seatedEyeOffset = ClampF(ReadFloat(iniPath, "SeatedEyeOffset", cfg.seatedEyeOffset), 0.0f, 0.60f);
        cfg.standingEyeHeight = ReadFloat(iniPath, "StandingEyeHeight", cfg.standingEyeHeight);
        if (cfg.standingEyeHeight < 0.0f) cfg.standingEyeHeight = 0.0f;
        cfg.worldScale = ClampF(ReadFloat(iniPath, "WorldScale", cfg.worldScale), 0.5f, 2.0f);
        cfg.southpaw = ReadInt(iniPath, "Southpaw", cfg.southpaw) ? 1 : 0;
        int debounce = ReadInt(iniPath, "GestureDebounceMs", cfg.gestureDebounceMs);
        if (debounce < 120) debounce = 120;
        if (debounce > 250) debounce = 250;
        cfg.gestureDebounceMs = debounce;
        // GH #40: diegetic ammo quad crop rect (INI-tunable per resolution/weapon)
        cfg.ammoQuadCropX = ClampF(ReadFloat(iniPath, "AmmoQuadCropX", cfg.ammoQuadCropX), 0.0f, 1.0f);
        cfg.ammoQuadCropY = ClampF(ReadFloat(iniPath, "AmmoQuadCropY", cfg.ammoQuadCropY), 0.0f, 1.0f);
        cfg.ammoQuadCropW = ClampF(ReadFloat(iniPath, "AmmoQuadCropW", cfg.ammoQuadCropW), 0.01f, 1.0f);
        cfg.ammoQuadCropH = ClampF(ReadFloat(iniPath, "AmmoQuadCropH", cfg.ammoQuadCropH), 0.01f, 1.0f);
        cfg.wristHudCropW = ClampF(ReadFloat(iniPath, "WristHudCropW", cfg.wristHudCropW), 0.01f, 1.0f);
        cfg.wristHudCropH = ClampF(ReadFloat(iniPath, "WristHudCropH", cfg.wristHudCropH), 0.01f, 1.0f);
        // GH #46: native arm/hand mesh draw (provisional; default off).
        cfg.nativeHands = ReadInt(iniPath, "NativeHands", cfg.nativeHands) ? 1 : 0;
        cfg.nativeHandScale = ClampF(ReadFloat(iniPath, "NativeHandScale", cfg.nativeHandScale), 0.005f, 0.20f);
        cfg.nativeHandOffsetX = ClampF(ReadFloat(iniPath, "NativeHandOffsetX", cfg.nativeHandOffsetX), -1.0f, 1.0f);
        cfg.nativeHandOffsetY = ClampF(ReadFloat(iniPath, "NativeHandOffsetY", cfg.nativeHandOffsetY), -1.0f, 1.0f);
        cfg.nativeHandOffsetZ = ClampF(ReadFloat(iniPath, "NativeHandOffsetZ", cfg.nativeHandOffsetZ), -1.0f, 1.0f);
        // GH #61: bake texture bind (provisional; the flat skin tone stays the
        // fallback even when the texture is on but unavailable).
        cfg.nativeHandTexture = ReadInt(iniPath, "NativeHandTexture", cfg.nativeHandTexture) ? 1 : 0;
        cfg.nativeHandTextureVFlip = ReadInt(iniPath, "NativeHandTextureVFlip", cfg.nativeHandTextureVFlip) ? 1 : 0;
        // GH #62: front-of-HMD block/guard envelope (head-local metres). The
        // guard must sit in front of the face, not below the view, so Y is
        // clamped well above the old -0.42 m low-hold floor.
        cfg.guardYMin = ClampF(ReadFloat(iniPath, "GuardYMin", cfg.guardYMin), -0.40f, 0.20f);
        cfg.guardYMax = ClampF(ReadFloat(iniPath, "GuardYMax", cfg.guardYMax), -0.05f, 0.60f);
        cfg.guardZMin = ClampF(ReadFloat(iniPath, "GuardZMin", cfg.guardZMin), -1.00f, -0.20f);
        cfg.guardZMax = ClampF(ReadFloat(iniPath, "GuardZMax", cfg.guardZMax), -0.55f, -0.02f);
        cfg.guardMaxHandSpan = ClampF(ReadFloat(iniPath, "GuardMaxHandSpan", cfg.guardMaxHandSpan), 0.10f, 1.00f);
        // Reject an inverted window (e.g. GuardYMin > GuardYMax) so a typo can
        // never make the guard pose unreachable.
        if (cfg.guardYMin > cfg.guardYMax) cfg.guardYMin = cfg.guardYMax;
        if (cfg.guardZMin > cfg.guardZMax) cfg.guardZMax = cfg.guardZMin;
        VR_LOG_INFO("VR config loaded from %s (snap=%.0f move_dz=%.2f comfort=%d seated=%d scale=%.2f paw=%d native_hands=%d native_tex=%d guard_y=[%.2f,%.2f] guard_z=[%.2f,%.2f] guard_span=%.2f)",
            iniPath, cfg.snapAngleDeg, cfg.moveDeadzone, cfg.comfortEnabled,
            cfg.seated, cfg.worldScale, cfg.southpaw, cfg.nativeHands,
            cfg.nativeHandTexture, cfg.guardYMin, cfg.guardYMax,
            cfg.guardZMin, cfg.guardZMax, cfg.guardMaxHandSpan);
    } else {
        VR_LOG_INFO("VR config: no riddick_vr.ini found, using defaults");
    }

    g_vrConfig = cfg;
}

} // namespace RiddickVR
