#pragma once

#include <windows.h>
#include <GL/gl.h>

namespace RiddickVR {

// Forward-declared modern OpenGL extension function types
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
typedef void (APIENTRY *PFNGLBLITFRAMEBUFFERPROC)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef void (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

typedef void (APIENTRY *PFN_glBindTexture)(GLenum target, GLuint texture);
typedef void (APIENTRY *PFN_glGenTextures)(GLsizei n, GLuint *textures);
typedef void (APIENTRY *PFN_glDeleteTextures)(GLsizei n, const GLuint *textures);
typedef void (APIENTRY *PFN_glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRY *PFN_glTexParameteri)(GLenum target, GLenum pname, GLint param);

class OpenGLProxy {
public:
    static bool Initialize();
    static void Shutdown();

    // WGL Interception Points
    static HGLRC WINAPI Hook_wglCreateContext(HDC hdc);
    static BOOL  WINAPI Hook_wglDeleteContext(HGLRC hglrc);
    static BOOL  WINAPI Hook_wglMakeCurrent(HDC hdc, HGLRC hglrc);
    static BOOL  WINAPI Hook_wglSwapBuffers(HDC hdc);
    static PROC  WINAPI Hook_wglGetProcAddress(LPCSTR lpszProc);

    // GL Interception Points
    static void WINAPI Hook_glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
    static void WINAPI Hook_glScissor(GLint x, GLint y, GLsizei width, GLsizei height);

    // GL 1.1 functions retrieved dynamically
    static PFN_glBindTexture glBindTexture;
    static PFN_glGenTextures glGenTextures;
    static PFN_glDeleteTextures glDeleteTextures;
    static PFN_glTexImage2D glTexImage2D;
    static PFN_glTexParameteri glTexParameteri;

    // Extension pointers retrieved via wglGetProcAddress
    static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
    static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
    static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
    static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
    static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
    static PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
    static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;

    static HMODULE GetRealOpenGLModule() { return s_hRealOpenGL; }

private:
    static HMODULE s_hRealOpenGL;
    static bool s_initialized;
};

} // namespace RiddickVR
