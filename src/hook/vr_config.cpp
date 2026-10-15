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
        VR_LOG_INFO("VR config loaded from %s (snap=%.0f move_dz=%.2f comfort=%d seated=%d scale=%.2f paw=%d)",
            iniPath, cfg.snapAngleDeg, cfg.moveDeadzone, cfg.comfortEnabled,
            cfg.seated, cfg.worldScale, cfg.southpaw);
    } else {
        VR_LOG_INFO("VR config: no riddick_vr.ini found, using defaults");
    }

    g_vrConfig = cfg;
}

} // namespace RiddickVR
