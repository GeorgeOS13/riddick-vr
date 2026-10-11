#include "opengl_proxy.h"
#include "stereo_renderer.h"
#include "logging.h"
#include <string>

namespace RiddickVR {

HMODULE OpenGLProxy::s_hRealOpenGL = nullptr;
bool OpenGLProxy::s_initialized = false;

// Extension pointers
PFNGLBINDFRAMEBUFFERPROC OpenGLProxy::glBindFramebuffer = nullptr;
PFNGLGENFRAMEBUFFERSPROC OpenGLProxy::glGenFramebuffers = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC OpenGLProxy::glDeleteFramebuffers = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC OpenGLProxy::glFramebufferTexture2D = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC OpenGLProxy::glCheckFramebufferStatus = nullptr;
PFNGLBLITFRAMEBUFFERPROC OpenGLProxy::glBlitFramebuffer = nullptr;
PFNGLUNIFORMMATRIX4FVPROC OpenGLProxy::glUniformMatrix4fv = nullptr;

PFN_glBindTexture OpenGLProxy::glBindTexture = nullptr;
PFN_glGenTextures OpenGLProxy::glGenTextures = nullptr;
PFN_glDeleteTextures OpenGLProxy::glDeleteTextures = nullptr;
PFN_glTexImage2D OpenGLProxy::glTexImage2D = nullptr;
PFN_glTexParameteri OpenGLProxy::glTexParameteri = nullptr;

// Real WGL Typedefs
typedef HGLRC (WINAPI *PFN_wglCreateContext)(HDC);
typedef BOOL  (WINAPI *PFN_wglDeleteContext)(HGLRC);
typedef BOOL  (WINAPI *PFN_wglMakeCurrent)(HDC, HGLRC);
typedef BOOL  (WINAPI *PFN_wglSwapBuffers)(HDC);
typedef PROC  (WINAPI *PFN_wglGetProcAddress)(LPCSTR);
typedef void  (WINAPI *PFN_glViewport)(GLint, GLint, GLsizei, GLsizei);
typedef void  (WINAPI *PFN_glScissor)(GLint, GLint, GLsizei, GLsizei);

static PFN_wglCreateContext real_wglCreateContext = nullptr;
static PFN_wglDeleteContext real_wglDeleteContext = nullptr;
static PFN_wglMakeCurrent   real_wglMakeCurrent = nullptr;
static PFN_wglSwapBuffers   real_wglSwapBuffers = nullptr;
static PFN_wglGetProcAddress real_wglGetProcAddress = nullptr;
static PFN_glViewport       real_glViewport = nullptr;
static PFN_glScissor        real_glScissor = nullptr;

bool OpenGLProxy::Initialize() {
    if (s_initialized) return true;

    char sysPath[MAX_PATH];
    GetSystemDirectoryA(sysPath, MAX_PATH);
    std::string realDllPath = std::string(sysPath) + "\\opengl32.dll";

    VR_LOG_INFO("Loading real OpenGL from: %s", realDllPath.c_str());
    s_hRealOpenGL = LoadLibraryA(realDllPath.c_str());
    if (!s_hRealOpenGL) {
        VR_LOG_ERROR("Failed to load real system opengl32.dll! Error: %lu", GetLastError());
        return false;
    }

    real_wglCreateContext = (PFN_wglCreateContext)GetProcAddress(s_hRealOpenGL, "wglCreateContext");
    real_wglDeleteContext = (PFN_wglDeleteContext)GetProcAddress(s_hRealOpenGL, "wglDeleteContext");
    real_wglMakeCurrent   = (PFN_wglMakeCurrent)GetProcAddress(s_hRealOpenGL, "wglMakeCurrent");
    real_wglSwapBuffers   = (PFN_wglSwapBuffers)GetProcAddress(s_hRealOpenGL, "wglSwapBuffers");
    real_wglGetProcAddress = (PFN_wglGetProcAddress)GetProcAddress(s_hRealOpenGL, "wglGetProcAddress");
    real_glViewport       = (PFN_glViewport)GetProcAddress(s_hRealOpenGL, "glViewport");
    real_glScissor        = (PFN_glScissor)GetProcAddress(s_hRealOpenGL, "glScissor");

    glBindTexture         = (PFN_glBindTexture)GetProcAddress(s_hRealOpenGL, "glBindTexture");
    glGenTextures         = (PFN_glGenTextures)GetProcAddress(s_hRealOpenGL, "glGenTextures");
    glDeleteTextures      = (PFN_glDeleteTextures)GetProcAddress(s_hRealOpenGL, "glDeleteTextures");
    glTexImage2D          = (PFN_glTexImage2D)GetProcAddress(s_hRealOpenGL, "glTexImage2D");
    glTexParameteri       = (PFN_glTexParameteri)GetProcAddress(s_hRealOpenGL, "glTexParameteri");

    s_initialized = true;
    VR_LOG_INFO("OpenGLProxy initialized successfully.");
    return true;
}

void OpenGLProxy::Shutdown() {
    if (s_hRealOpenGL) {
        FreeLibrary(s_hRealOpenGL);
        s_hRealOpenGL = nullptr;
    }
    s_initialized = false;
}

HGLRC WINAPI OpenGLProxy::Hook_wglCreateContext(HDC hdc) {
    if (!s_initialized) Initialize();
    HGLRC result = real_wglCreateContext ? real_wglCreateContext(hdc) : nullptr;
    VR_LOG_INFO("wglCreateContext called (HDC: %p -> HGLRC: %p)", hdc, result);
    return result;
}

BOOL WINAPI OpenGLProxy::Hook_wglDeleteContext(HGLRC hglrc) {
    VR_LOG_INFO("wglDeleteContext called for HGLRC: %p", hglrc);
    StereoRenderer::Instance().Shutdown();
    return real_wglDeleteContext ? real_wglDeleteContext(hglrc) : FALSE;
}

BOOL WINAPI OpenGLProxy::Hook_wglMakeCurrent(HDC hdc, HGLRC hglrc) {
    BOOL result = real_wglMakeCurrent ? real_wglMakeCurrent(hdc, hglrc) : FALSE;
    if (result && hglrc != nullptr) {
        StereoRenderer::Instance().Initialize(hdc, hglrc);
    }
    return result;
}

BOOL WINAPI OpenGLProxy::Hook_wglSwapBuffers(HDC hdc) {
    StereoRenderer::Instance().OnSwapBuffers(hdc);
    return real_wglSwapBuffers ? real_wglSwapBuffers(hdc) : SwapBuffers(hdc);
}

PROC WINAPI OpenGLProxy::Hook_wglGetProcAddress(LPCSTR lpszProc) {
    if (!lpszProc) return nullptr;

    PROC realProc = real_wglGetProcAddress ? real_wglGetProcAddress(lpszProc) : nullptr;

    // Cache essential extension pointers when requested by the game engine
    std::string procName(lpszProc);
    if (procName == "glBindFramebuffer" || procName == "glBindFramebufferEXT") {
        glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)realProc;
    } else if (procName == "glGenFramebuffers" || procName == "glGenFramebuffersEXT") {
        glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)realProc;
    } else if (procName == "glDeleteFramebuffers" || procName == "glDeleteFramebuffersEXT") {
        glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)realProc;
    } else if (procName == "glFramebufferTexture2D" || procName == "glFramebufferTexture2DEXT") {
        glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)realProc;
    } else if (procName == "glCheckFramebufferStatus" || procName == "glCheckFramebufferStatusEXT") {
        glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)realProc;
    } else if (procName == "glBlitFramebuffer" || procName == "glBlitFramebufferEXT") {
        glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)realProc;
    } else if (procName == "glUniformMatrix4fv" || procName == "glUniformMatrix4fvARB") {
        glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)realProc;
    }

    return realProc;
}

void WINAPI OpenGLProxy::Hook_glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    StereoRenderer::Instance().OnViewport(x, y, width, height);
    if (real_glViewport) {
        real_glViewport(x, y, width, height);
    }
}

void WINAPI OpenGLProxy::Hook_glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    if (real_glScissor) {
        real_glScissor(x, y, width, height);
    }
}

} // namespace RiddickVR
