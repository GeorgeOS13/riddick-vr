#include <windows.h>
#include "logging.h"
#include "opengl_proxy.h"
#include "camera_hook.h"
#include "stereo_renderer.h"

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

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            DisableThreadLibraryCalls(hModule);
            RiddickVR::Logger::Instance().Initialize("riddick_vr.log");
            VR_LOG_INFO("Riddick VR Hook DLL Attached (PID: %lu)", GetCurrentProcessId());

            if (!RiddickVR::OpenGLProxy::Initialize()) {
                VR_LOG_ERROR("Failed to initialize OpenGL Proxy! Aborting hook setup.");
                return FALSE;
            }

            RiddickVR::CameraHook::Instance().Initialize();
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
