#include "opengl_proxy.h"
#include "opengl_forwarders.h"
#include "stereo_renderer.h"
#include "camera_hook.h"
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

PFNGLGENRENDERBUFFERSPROC OpenGLProxy::glGenRenderbuffers = nullptr;
PFNGLBINDRENDERBUFFERPROC OpenGLProxy::glBindRenderbuffer = nullptr;
PFNGLRENDERBUFFERSTORAGEPROC OpenGLProxy::glRenderbufferStorage = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC OpenGLProxy::glFramebufferRenderbuffer = nullptr;
PFNGLDELETERENDERBUFFERSPROC OpenGLProxy::glDeleteRenderbuffers = nullptr;

PFNGLPROGRAMENVPARAMETER4FVARBPROC OpenGLProxy::glProgramEnvParameter4fvARB = nullptr;
PFNGLPROGRAMLOCALPARAMETER4FVARBPROC OpenGLProxy::glProgramLocalParameter4fvARB = nullptr;

PFNWGLDXOPENDEVICENVPROC OpenGLProxy::wglDXOpenDeviceNV = nullptr;
PFNWGLDXCLOSEDEVICENVPROC OpenGLProxy::wglDXCloseDeviceNV = nullptr;
PFNWGLDXREGISTEROBJECTNVPROC OpenGLProxy::wglDXRegisterObjectNV = nullptr;
PFNWGLDXUNREGISTEROBJECTNVPROC OpenGLProxy::wglDXUnregisterObjectNV = nullptr;
PFNWGLDXOBJECTACCESSNVPROC OpenGLProxy::wglDXObjectAccessNV = nullptr;
PFNWGLDXLOCKOBJECTSNVPROC OpenGLProxy::wglDXLockObjectsNV = nullptr;
PFNWGLDXUNLOCKOBJECTSNVPROC OpenGLProxy::wglDXUnlockObjectsNV = nullptr;

PFN_glBindTexture OpenGLProxy::glBindTexture = nullptr;
PFN_glGenTextures OpenGLProxy::glGenTextures = nullptr;
PFN_glDeleteTextures OpenGLProxy::glDeleteTextures = nullptr;
PFN_glTexImage2D OpenGLProxy::glTexImage2D = nullptr;
PFN_glTexParameteri OpenGLProxy::glTexParameteri = nullptr;

// Real WGL & GL Typedefs
typedef HGLRC (WINAPI *PFN_wglCreateContext)(HDC);
typedef BOOL  (WINAPI *PFN_wglDeleteContext)(HGLRC);
typedef BOOL  (WINAPI *PFN_wglMakeCurrent)(HDC, HGLRC);
typedef BOOL  (WINAPI *PFN_wglSwapBuffers)(HDC);
typedef PROC  (WINAPI *PFN_wglGetProcAddress)(LPCSTR);
typedef void  (WINAPI *PFN_glViewport)(GLint, GLint, GLsizei, GLsizei);
typedef void  (WINAPI *PFN_glScissor)(GLint, GLint, GLsizei, GLsizei);
typedef void  (WINAPI *PFN_glMatrixMode)(GLenum);
typedef void  (WINAPI *PFN_glLoadMatrixf)(const GLfloat*);
typedef void  (WINAPI *PFN_glFrustum)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void  (WINAPI *PFN_glOrtho)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void  (WINAPI *PFN_glClear)(GLbitfield);

static PFN_wglCreateContext real_wglCreateContext = nullptr;
static PFN_wglDeleteContext real_wglDeleteContext = nullptr;
static PFN_wglMakeCurrent   real_wglMakeCurrent = nullptr;
static PFN_wglSwapBuffers   real_wglSwapBuffers = nullptr;
static PFN_wglGetProcAddress real_wglGetProcAddress = nullptr;
static PFN_glViewport       real_glViewport = nullptr;
static PFN_glScissor        real_glScissor = nullptr;
static PFN_glMatrixMode     real_glMatrixMode = nullptr;
static PFN_glLoadMatrixf    real_glLoadMatrixf = nullptr;
static PFN_glFrustum        real_glFrustum = nullptr;
static PFN_glOrtho          real_glOrtho = nullptr;
static PFN_glClear          real_glClear = nullptr;
static PFNGLBINDFRAMEBUFFERPROC real_glBindFramebuffer = nullptr;

static GLenum s_currentMatrixMode = 0x1700; // GL_MODELVIEW
static float s_lastProjection[16] = {0};
static float s_lastView[16] = {0};
static bool s_hasProjection = false;
static uint32_t s_projMatrixCount = 0;
static uint32_t s_modelviewMatrixCount = 0;

static void RawProxyLog(const char* msg) {
    HANDLE hFile = CreateFileA("C:\\Users\\GigelCastel\\riddick-vr\\hook_debug.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(hFile, msg, (DWORD)strlen(msg), &written, NULL);
        WriteFile(hFile, "\r\n", 2, &written, NULL);
        CloseHandle(hFile);
    }
}

bool OpenGLProxy::Initialize() {
    if (s_initialized) return true;

    char sysPath[MAX_PATH];
    GetSystemDirectoryA(sysPath, MAX_PATH);
    std::string realDllPath = std::string(sysPath) + "\\opengl32.dll";

    char logBuf[512];
    sprintf_s(logBuf, "OpenGLProxy: sysPath='%s', realDllPath='%s'", sysPath, realDllPath.c_str());
    RawProxyLog(logBuf);

    s_hRealOpenGL = LoadLibraryA(realDllPath.c_str());
    if (!s_hRealOpenGL) {
        DWORD err = GetLastError();
        sprintf_s(logBuf, "OpenGLProxy: LoadLibraryA failed with error %lu!", err);
        RawProxyLog(logBuf);
        VR_LOG_ERROR("Failed to load real system opengl32.dll! Error: %lu", err);
        return false;
    }
    char buf[128];
    sprintf_s(buf, "OpenGLProxy: real opengl32 loaded at %p", (void*)s_hRealOpenGL);
    RawProxyLog(buf);

    real_wglCreateContext = (PFN_wglCreateContext)GetProcAddress(s_hRealOpenGL, "wglCreateContext");
    real_wglDeleteContext = (PFN_wglDeleteContext)GetProcAddress(s_hRealOpenGL, "wglDeleteContext");
    real_wglMakeCurrent   = (PFN_wglMakeCurrent)GetProcAddress(s_hRealOpenGL, "wglMakeCurrent");
    real_wglSwapBuffers   = (PFN_wglSwapBuffers)GetProcAddress(s_hRealOpenGL, "wglSwapBuffers");
    real_wglGetProcAddress = (PFN_wglGetProcAddress)GetProcAddress(s_hRealOpenGL, "wglGetProcAddress");
    real_glViewport       = (PFN_glViewport)GetProcAddress(s_hRealOpenGL, "glViewport");
    real_glScissor        = (PFN_glScissor)GetProcAddress(s_hRealOpenGL, "glScissor");
    real_glMatrixMode     = (PFN_glMatrixMode)GetProcAddress(s_hRealOpenGL, "glMatrixMode");
    real_glLoadMatrixf    = (PFN_glLoadMatrixf)GetProcAddress(s_hRealOpenGL, "glLoadMatrixf");
    real_glFrustum        = (PFN_glFrustum)GetProcAddress(s_hRealOpenGL, "glFrustum");
    real_glOrtho          = (PFN_glOrtho)GetProcAddress(s_hRealOpenGL, "glOrtho");
    real_glClear          = (PFN_glClear)GetProcAddress(s_hRealOpenGL, "glClear");

    glBindTexture         = (PFN_glBindTexture)GetProcAddress(s_hRealOpenGL, "glBindTexture");
    glGenTextures         = (PFN_glGenTextures)GetProcAddress(s_hRealOpenGL, "glGenTextures");
    glDeleteTextures      = (PFN_glDeleteTextures)GetProcAddress(s_hRealOpenGL, "glDeleteTextures");
    glTexImage2D          = (PFN_glTexImage2D)GetProcAddress(s_hRealOpenGL, "glTexImage2D");
    glTexParameteri       = (PFN_glTexParameteri)GetProcAddress(s_hRealOpenGL, "glTexParameteri");

    InitializeForwarders(s_hRealOpenGL);

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

void OpenGLProxy::QueryContextExtensions() {
    if (!real_wglGetProcAddress) return;

    auto LoadExt = [](const char* name1, const char* name2 = nullptr) -> PROC {
        PROC p = real_wglGetProcAddress(name1);
        if (!p && name2) p = real_wglGetProcAddress(name2);
        return p;
    };

    if (!real_glBindFramebuffer) {
        real_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)LoadExt("glBindFramebuffer", "glBindFramebufferEXT");
        glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)Hook_glBindFramebuffer;
    }
    if (!glGenFramebuffers) glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)LoadExt("glGenFramebuffers", "glGenFramebuffersEXT");
    if (!glDeleteFramebuffers) glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)LoadExt("glDeleteFramebuffers", "glDeleteFramebuffersEXT");
    if (!glFramebufferTexture2D) glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)LoadExt("glFramebufferTexture2D", "glFramebufferTexture2DEXT");
    if (!glCheckFramebufferStatus) glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)LoadExt("glCheckFramebufferStatus", "glCheckFramebufferStatusEXT");
    if (!glBlitFramebuffer) glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)LoadExt("glBlitFramebuffer", "glBlitFramebufferEXT");
    if (!glUniformMatrix4fv) glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)LoadExt("glUniformMatrix4fv", "glUniformMatrix4fvARB");

    if (!glGenRenderbuffers) glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)LoadExt("glGenRenderbuffers", "glGenRenderbuffersEXT");
    if (!glBindRenderbuffer) glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)LoadExt("glBindRenderbuffer", "glBindRenderbufferEXT");
    if (!glRenderbufferStorage) glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)LoadExt("glRenderbufferStorage", "glRenderbufferStorageEXT");
    if (!glFramebufferRenderbuffer) glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)LoadExt("glFramebufferRenderbuffer", "glFramebufferRenderbufferEXT");
    if (!glDeleteRenderbuffers) glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)LoadExt("glDeleteRenderbuffers", "glDeleteRenderbuffersEXT");

    if (!glProgramEnvParameter4fvARB) glProgramEnvParameter4fvARB = (PFNGLPROGRAMENVPARAMETER4FVARBPROC)LoadExt("glProgramEnvParameter4fvARB");
    if (!glProgramLocalParameter4fvARB) glProgramLocalParameter4fvARB = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)LoadExt("glProgramLocalParameter4fvARB");

    // Query WGL_NV_DX_interop
    if (!wglDXOpenDeviceNV) wglDXOpenDeviceNV = (PFNWGLDXOPENDEVICENVPROC)LoadExt("wglDXOpenDeviceNV");
    if (!wglDXCloseDeviceNV) wglDXCloseDeviceNV = (PFNWGLDXCLOSEDEVICENVPROC)LoadExt("wglDXCloseDeviceNV");
    if (!wglDXRegisterObjectNV) wglDXRegisterObjectNV = (PFNWGLDXREGISTEROBJECTNVPROC)LoadExt("wglDXRegisterObjectNV");
    if (!wglDXUnregisterObjectNV) wglDXUnregisterObjectNV = (PFNWGLDXUNREGISTEROBJECTNVPROC)LoadExt("wglDXUnregisterObjectNV");
    if (!wglDXObjectAccessNV) wglDXObjectAccessNV = (PFNWGLDXOBJECTACCESSNVPROC)LoadExt("wglDXObjectAccessNV");
    if (!wglDXLockObjectsNV) wglDXLockObjectsNV = (PFNWGLDXLOCKOBJECTSNVPROC)LoadExt("wglDXLockObjectsNV");
    if (!wglDXUnlockObjectsNV) wglDXUnlockObjectsNV = (PFNWGLDXUNLOCKOBJECTSNVPROC)LoadExt("wglDXUnlockObjectsNV");

    if (wglDXOpenDeviceNV && wglDXRegisterObjectNV) {
        VR_LOG_INFO("Hardware Interop: WGL_NV_DX_interop supported by active OpenGL driver!");
    } else {
        VR_LOG_INFO("Hardware Interop: WGL_NV_DX_interop not advertised; using FBO blit pipeline.");
    }
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
        QueryContextExtensions();
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
        real_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)realProc;
        glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)Hook_glBindFramebuffer;
        return (PROC)Hook_glBindFramebuffer;
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

void WINAPI OpenGLProxy::Hook_glMatrixMode(GLenum mode) {
    s_currentMatrixMode = mode;
    if (real_glMatrixMode) {
        real_glMatrixMode(mode);
    }
}

void WINAPI OpenGLProxy::Hook_glLoadMatrixf(const GLfloat *m) {
    if (!m) return;

    if (s_currentMatrixMode == 0x1701 /* GL_PROJECTION */) {
        memcpy(s_lastProjection, m, sizeof(s_lastProjection));
        s_hasProjection = true;
        s_projMatrixCount++;
        if (s_projMatrixCount == 1 || s_projMatrixCount % 300 == 0) {
            VR_LOG_INFO("glLoadMatrixf [PROJECTION #%u]: [%.2f, %.2f, %.2f, %.2f] [%.2f, %.2f, %.2f, %.2f]",
                s_projMatrixCount, m[0], m[5], m[10], m[14], m[1], m[2], m[3], m[15]);
        }

        // 3D Perspective Projection Interception (m[15] == 0.0f)
        if (StereoRenderer::Instance().IsVREnabled() && fabsf(m[15]) < 0.01f) {
            const auto& vrState = StereoRenderer::Instance().GetLatestState();
            const auto pass = StereoRenderer::Instance().GetCurrentPass();

            GLfloat vrProj[16];
            const float* srcProj = (pass == StereoPass::RightEye) 
                ? vrState.rightEyeProjection.m 
                : vrState.leftEyeProjection.m;
            memcpy(vrProj, srcProj, sizeof(vrProj));

            // Preserve near/far planes from original game projection if valid
            if (m[10] != 0.0f && m[14] != 0.0f) {
                vrProj[10] = m[10];
                vrProj[14] = m[14];
            }

            if (real_glLoadMatrixf) {
                real_glLoadMatrixf(vrProj);
                return;
            }
        }
        // 2D Orthographic HUD / UI Projection Interception (fabsf(m[15] - 1.0f) < 0.01f)
        else if (StereoRenderer::Instance().IsVREnabled() && fabsf(m[15] - 1.0f) < 0.01f) {
            const auto pass = StereoRenderer::Instance().GetCurrentPass();
            float hudOffset = StereoRenderer::Instance().GetHudConvergenceOffset();

            GLfloat hudProj[16];
            memcpy(hudProj, m, sizeof(hudProj));

            // Apply slight horizontal convergence parallax for stereoscopic depth (~1.8m virtual plane)
            if (pass == StereoPass::LeftEye) {
                hudProj[12] += hudOffset;
            } else if (pass == StereoPass::RightEye) {
                hudProj[12] -= hudOffset;
            }

            if (real_glLoadMatrixf) {
                real_glLoadMatrixf(hudProj);
                return;
            }
        }
    } else if (s_currentMatrixMode == 0x1700 /* GL_MODELVIEW */) {
        memcpy(s_lastView, m, sizeof(s_lastView));
        s_modelviewMatrixCount++;
        if (s_modelviewMatrixCount == 1 || s_modelviewMatrixCount % 300 == 0) {
            VR_LOG_INFO("glLoadMatrixf [MODELVIEW #%u]: Pos(%.2f, %.2f, %.2f)",
                s_modelviewMatrixCount, m[12], m[13], m[14]);
        }

        // 3D Modelview Camera Interception with Horizon Stabilization
        if (StereoRenderer::Instance().IsVREnabled()) {
            const auto& vrState = StereoRenderer::Instance().GetLatestState();
            const auto pass = StereoRenderer::Instance().GetCurrentPass();

            GLfloat stabilizedView[16];
            CameraHook::Instance().StabilizeViewMatrix(
                stabilizedView, 
                m, 
                vrState, 
                (pass == StereoPass::RightEye ? 2 : 1)
            );

            if (real_glLoadMatrixf) {
                real_glLoadMatrixf(stabilizedView);
                return;
            }
        }
    }

    if (real_glLoadMatrixf) {
        real_glLoadMatrixf(m);
    }
}

void WINAPI OpenGLProxy::Hook_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    VR_LOG_INFO("glFrustum called: L=%.2f R=%.2f B=%.2f T=%.2f Near=%.2f Far=%.2f", left, right, bottom, top, zNear, zFar);
    if (real_glFrustum) {
        real_glFrustum(left, right, bottom, top, zNear, zFar);
    }
}

void WINAPI OpenGLProxy::Hook_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    if (StereoRenderer::Instance().IsVREnabled()) {
        const auto pass = StereoRenderer::Instance().GetCurrentPass();
        double hudOffset = (right - left) * StereoRenderer::Instance().GetHudConvergenceOffset();
        if (pass == StereoPass::LeftEye) {
            left += hudOffset;
            right += hudOffset;
        } else if (pass == StereoPass::RightEye) {
            left -= hudOffset;
            right -= hudOffset;
        }
    }
    if (real_glOrtho) {
        real_glOrtho(left, right, bottom, top, zNear, zFar);
    }
}

void WINAPI OpenGLProxy::Hook_glClear(GLbitfield mask) {
    if (StereoRenderer::Instance().IsVREnabled()) {
        StereoRenderer::Instance().OnClear(mask);
    }
    if (real_glClear) {
        real_glClear(mask);
    }
}

void WINAPI OpenGLProxy::Hook_glBindFramebuffer(GLenum target, GLuint framebuffer) {
    if (StereoRenderer::Instance().IsVREnabled()) {
        if (framebuffer == 0) {
            GLuint eyeFbo = StereoRenderer::Instance().GetActiveEyeFbo();
            if (eyeFbo && real_glBindFramebuffer) {
                real_glBindFramebuffer(target, eyeFbo);
                return;
            }
        }
    }
    if (real_glBindFramebuffer) {
        real_glBindFramebuffer(target, framebuffer);
    }
}

GLenum OpenGLProxy::GetCurrentMatrixMode() {
    return s_currentMatrixMode;
}

bool OpenGLProxy::HasInterceptedProjection() {
    return s_hasProjection;
}

const float* OpenGLProxy::GetLastProjectionMatrix() {
    return s_lastProjection;
}

const float* OpenGLProxy::GetLastViewMatrix() {
    return s_lastView;
}

} // namespace RiddickVR
