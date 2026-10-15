#include <windows.h>
#include "logging.h"
#include "opengl_proxy.h"
#include "camera_hook.h"
#include "stereo_renderer.h"
#include "iat_hook.h"

// WGL exports hooked directly
extern "C" {

HGLRC WINAPI wglCreateContext(HDC hdc) {
    return RiddickVR::OpenGLProxy::Hook_wglCreateContext(hdc);
}

BOOL WINAPI wglDeleteContext(HGLRC hglrc) {
    return RiddickVR::OpenGLProxy::Hook_wglDeleteContext(hglrc);
}

BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hglrc) {
    return RiddickVR::OpenGLProxy::Hook_wglMakeCurrent(hdc, hglrc);
}

BOOL WINAPI wglSwapBuffers(HDC hdc) {
    return RiddickVR::OpenGLProxy::Hook_wglSwapBuffers(hdc);
}

PROC WINAPI wglGetProcAddress(LPCSTR lpszProc) {
    return RiddickVR::OpenGLProxy::Hook_wglGetProcAddress(lpszProc);
}

void WINAPI glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    RiddickVR::OpenGLProxy::Hook_glViewport(x, y, width, height);
}

void WINAPI glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    RiddickVR::OpenGLProxy::Hook_glScissor(x, y, width, height);
}

void WINAPI glMatrixMode(GLenum mode) {
    RiddickVR::OpenGLProxy::Hook_glMatrixMode(mode);
}

void WINAPI glLoadMatrixf(const GLfloat* m) {
    RiddickVR::OpenGLProxy::Hook_glLoadMatrixf(m);
}

void WINAPI glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    RiddickVR::OpenGLProxy::Hook_glFrustum(left, right, bottom, top, zNear, zFar);
}

void WINAPI glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    RiddickVR::OpenGLProxy::Hook_glOrtho(left, right, bottom, top, zNear, zFar);
}

void WINAPI glClear(GLbitfield mask) {
    RiddickVR::OpenGLProxy::Hook_glClear(mask);
}

} // extern "C"

typedef BOOL (WINAPI *PFN_GdiSwapBuffers)(HDC);
static PFN_GdiSwapBuffers real_GdiSwapBuffers = nullptr;

static BOOL WINAPI Hook_GdiSwapBuffers(HDC hdc) {
    RiddickVR::StereoRenderer::Instance().OnSwapBuffers(hdc);
    return real_GdiSwapBuffers ? real_GdiSwapBuffers(hdc) : SwapBuffers(hdc);
}

static void InstallRndrGLHooks() {
    HMODULE hRndrGL = GetModuleHandleA("RndrGL.dll");
    if (hRndrGL) {
        RiddickVR::IATHook::Hook(
            hRndrGL,
            "GDI32.dll",
            "SwapBuffers",
            reinterpret_cast<void*>(Hook_GdiSwapBuffers),
            reinterpret_cast<void**>(&real_GdiSwapBuffers)
        );
    }
}

static HMODULE g_hHookModule = nullptr;
static LONG g_hookInitState = 0; // 0=uninitialized, 1=initializing, 2=ready/failed

static void HookDebugLogPath(char* outPath, DWORD outSize) {
    // GH #26: never write to a hardcoded developer path; %TEMP% first,
    // hook DLL directory as fallback.
    DWORD len = GetTempPathA(outSize, outPath);
    if (len > 0 && len < outSize) {
        strcat_s(outPath, outSize, "riddick_vr_hook_debug.txt");
        return;
    }
    if (g_hHookModule && GetModuleFileNameA(g_hHookModule, outPath, outSize) > 0) {
        char* lastSlash = strrchr(outPath, '\\');
        if (lastSlash) {
            *(lastSlash + 1) = '\0';
            strcat_s(outPath, outSize, "hook_debug.txt");
            return;
        }
    }
    strcpy_s(outPath, outSize, "hook_debug.txt");
}

static void RawDebugLog(const char* msg) {
    char path[MAX_PATH];
    HookDebugLogPath(path, MAX_PATH);
    HANDLE hFile = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(hFile, msg, (DWORD)strlen(msg), &written, NULL);
        WriteFile(hFile, "\r\n", 2, &written, NULL);
        CloseHandle(hFile);
    }
}

// GH #27: DllMain must not do LoadLibrary / IO / subsystem init under the
// loader lock. It only stashes the module handle; real init is deferred to
// the first wglCreateContext (worker/app thread) via EnsureHookInitialized.
void RiddickVR_EnsureHookInitialized();

static void EnsureHookInitializedImpl() {
    RawDebugLog("DeferredInit: entered");

    // Enable Per-Monitor V2 DPI awareness so Windows never downscales the game viewport
    SetProcessDPIAware();
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (hUser32) {
        typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);
        auto setDpiContext = (PFN_SetProcessDpiAwarenessContext)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (setDpiContext) {
            setDpiContext((HANDLE)-4 /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 */);
        }
    }

    RawDebugLog("DeferredInit: Initializing Logger");
    char logPath[MAX_PATH];
    if (g_hHookModule) {
        GetModuleFileNameA(g_hHookModule, logPath, MAX_PATH);
    } else {
        logPath[0] = '\0';
    }
    char* lastSlash = strrchr(logPath, '\\');
    if (lastSlash) {
        *(lastSlash + 1) = '\0';
        strcat_s(logPath, "riddick_vr.log");
    } else {
        strcpy_s(logPath, "riddick_vr.log");
    }
    RiddickVR::Logger::Instance().Initialize(logPath);
    VR_LOG_INFO("Riddick VR Hook DLL Attached (PID: %lu)", GetCurrentProcessId());

    RawDebugLog("DeferredInit: Initializing OpenGLProxy");
    if (!RiddickVR::OpenGLProxy::Initialize()) {
        RawDebugLog("DeferredInit: OpenGLProxy::Initialize failed!");
        VR_LOG_ERROR("Failed to initialize OpenGL Proxy! Aborting hook setup.");
        return;
    }
    RawDebugLog("DeferredInit: OpenGLProxy::Initialize succeeded");

    RawDebugLog("DeferredInit: Installing RndrGL Hooks");
    InstallRndrGLHooks();
    RawDebugLog("DeferredInit: Initializing CameraHook");
    RiddickVR::CameraHook::Instance().Initialize();
    RawDebugLog("DeferredInit: Done");
}

void RiddickVR_EnsureHookInitialized() {
    // One-time, thread-safe-ish init without blocking the loader: first
    // caller wins via interlocked transition 0->1; losers spin briefly for 2.
    if (g_hookInitState == 2) return;
    if (InterlockedCompareExchange(&g_hookInitState, 1, 0) == 0) {
        EnsureHookInitializedImpl();
        InterlockedExchange(&g_hookInitState, 2);
    } else {
        for (int i = 0; i < 200 && g_hookInitState != 2; ++i) {
            Sleep(1);
        }
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            g_hHookModule = hModule;
            DisableThreadLibraryCalls(hModule);
            // Loader-lock-safe attach breadcrumb (OutputDebugString never touches
            // the loader lock; visible in DebugView/dbgview without file IO).
            // Answers "did the proxy DLL load?" when the game stalls pre-GL.
            OutputDebugStringA("RiddickVR: opengl32 proxy attached (init deferred to first wglCreateContext)\r\n");
            break;
        }
        case DLL_PROCESS_DETACH: {
            VR_LOG_INFO("Riddick VR Hook DLL Detaching...");
            RiddickVR::StereoRenderer::Instance().Shutdown();
            RiddickVR::OpenGLProxy::Shutdown();
            break;
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}
