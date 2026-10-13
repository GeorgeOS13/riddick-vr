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

static void RawDebugLog(const char* msg) {
    HANDLE hFile = CreateFileA("C:\\Users\\GigelCastel\\riddick-vr\\hook_debug.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(hFile, msg, (DWORD)strlen(msg), &written, NULL);
        WriteFile(hFile, "\r\n", 2, &written, NULL);
        CloseHandle(hFile);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            RawDebugLog("DllMain: DLL_PROCESS_ATTACH entered");
            DisableThreadLibraryCalls(hModule);
            
            RawDebugLog("DllMain: Initializing Logger");
            char logPath[MAX_PATH];
            GetModuleFileNameA(hModule, logPath, MAX_PATH);
            char* lastSlash = strrchr(logPath, '\\');
            if (lastSlash) {
                *(lastSlash + 1) = '\0';
                strcat_s(logPath, "riddick_vr.log");
            } else {
                strcpy_s(logPath, "riddick_vr.log");
            }
            RiddickVR::Logger::Instance().Initialize(logPath);
            VR_LOG_INFO("Riddick VR Hook DLL Attached (PID: %lu)", GetCurrentProcessId());

            RawDebugLog("DllMain: Initializing OpenGLProxy");
            if (!RiddickVR::OpenGLProxy::Initialize()) {
                RawDebugLog("DllMain: OpenGLProxy::Initialize failed!");
                VR_LOG_ERROR("Failed to initialize OpenGL Proxy! Aborting hook setup.");
                return FALSE;
            }
            RawDebugLog("DllMain: OpenGLProxy::Initialize succeeded");

            RawDebugLog("DllMain: Installing RndrGL Hooks");
            InstallRndrGLHooks();
            RawDebugLog("DllMain: Initializing CameraHook");
            RiddickVR::CameraHook::Instance().Initialize();
            RawDebugLog("DllMain: Done DLL_PROCESS_ATTACH");
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
