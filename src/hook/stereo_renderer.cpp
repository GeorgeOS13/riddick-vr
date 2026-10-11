#include "stereo_renderer.h"
#include "logging.h"

namespace RiddickVR {

StereoRenderer& StereoRenderer::Instance() {
    static StereoRenderer s_instance;
    return s_instance;
}

bool StereoRenderer::Initialize(HDC hdc, HGLRC hglrc) {
    if (m_initialized) return true;

    VR_LOG_INFO("Initializing StereoRenderer for OpenGL context %p...", hglrc);

    // Attempt connecting to the 64-bit OpenXR Host
    if (m_ipcClient.Connect(1500)) {
        VR_LOG_INFO("Connected to RiddickVRHost shared memory IPC bridge!");
        m_vrActive = true;
        if (m_ipcClient.ReadState(m_sharedState)) {
            if (m_sharedState.renderWidth > 0 && m_sharedState.renderHeight > 0) {
                m_eyeWidth = m_sharedState.renderWidth;
                m_eyeHeight = m_sharedState.renderHeight;
                VR_LOG_INFO("OpenXR requested eye resolution: %ux%u", m_eyeWidth, m_eyeHeight);
            }
        }
    } else {
        VR_LOG_WARN("Could not connect to RiddickVRHost (host process may not be running yet). VR standby mode active.");
        m_vrActive = false;
    }

    m_initialized = true;
    return true;
}

void StereoRenderer::Shutdown() {
    DestroyEyeBuffers();
    m_ipcClient.Disconnect();
    m_initialized = false;
    m_vrActive = false;
    VR_LOG_INFO("StereoRenderer shutdown complete.");
}

bool StereoRenderer::CreateEyeBuffers(uint32_t width, uint32_t height) {
    if (!OpenGLProxy::glGenFramebuffers) {
        VR_LOG_WARN("glGenFramebuffers pointer unavailable; cannot allocate custom eye FBOs yet.");
        return false;
    }

    DestroyEyeBuffers();

    m_eyeWidth = width;
    m_eyeHeight = height;

    // Allocate textures and FBOs for Left Eye
    OpenGLProxy::glGenFramebuffers(1, &m_leftFbo);
    OpenGLProxy::glGenTextures(1, &m_leftColorTex);
    OpenGLProxy::glBindTexture(GL_TEXTURE_2D, m_leftColorTex);
    OpenGLProxy::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    OpenGLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    OpenGLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    OpenGLProxy::glBindFramebuffer(0x8D40 /* GL_FRAMEBUFFER */, m_leftFbo);
    OpenGLProxy::glFramebufferTexture2D(0x8D40, 0x8CE0 /* GL_COLOR_ATTACHMENT0 */, GL_TEXTURE_2D, m_leftColorTex, 0);

    // Allocate textures and FBOs for Right Eye
    OpenGLProxy::glGenFramebuffers(1, &m_rightFbo);
    OpenGLProxy::glGenTextures(1, &m_rightColorTex);
    OpenGLProxy::glBindTexture(GL_TEXTURE_2D, m_rightColorTex);
    OpenGLProxy::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    OpenGLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    OpenGLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    OpenGLProxy::glBindFramebuffer(0x8D40, m_rightFbo);
    OpenGLProxy::glFramebufferTexture2D(0x8D40, 0x8CE0, GL_TEXTURE_2D, m_rightColorTex, 0);

    // Unbind back to default framebuffer
    OpenGLProxy::glBindFramebuffer(0x8D40, 0);
    OpenGLProxy::glBindTexture(GL_TEXTURE_2D, 0);

    VR_LOG_INFO("Stereo Eye Framebuffers successfully created (%ux%u)", width, height);
    return true;
}

void StereoRenderer::DestroyEyeBuffers() {
    if (OpenGLProxy::glDeleteFramebuffers) {
        if (m_leftFbo) {
            OpenGLProxy::glDeleteFramebuffers(1, &m_leftFbo);
            m_leftFbo = 0;
        }
        if (m_rightFbo) {
            OpenGLProxy::glDeleteFramebuffers(1, &m_rightFbo);
            m_rightFbo = 0;
        }
    }

    if (OpenGLProxy::glDeleteTextures) {
        if (m_leftColorTex) {
            OpenGLProxy::glDeleteTextures(1, &m_leftColorTex);
            m_leftColorTex = 0;
        }
        if (m_rightColorTex) {
            OpenGLProxy::glDeleteTextures(1, &m_rightColorTex);
            m_rightColorTex = 0;
        }
    }
}

void StereoRenderer::OnViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    m_windowWidth = width;
    m_windowHeight = height;
}

void StereoRenderer::OnSwapBuffers(HDC hdc) {
    if (!m_initialized) return;

    // Retry connecting to host if not yet connected
    if (!m_ipcClient.IsConnected()) {
        static DWORD s_lastRetry = 0;
        DWORD now = GetTickCount();
        if (now - s_lastRetry > 3000) {
            s_lastRetry = now;
            if (m_ipcClient.Connect(100)) {
                VR_LOG_INFO("Late connected to RiddickVRHost!");
                m_vrActive = true;
            }
        }
    }

    if (m_ipcClient.IsConnected()) {
        // Read updated tracking state for next frame
        if (m_ipcClient.ReadState(m_sharedState)) {
            m_vrActive = m_sharedState.vrActive;
        }

        // If buffers exist and blit is available, mirror left eye to default desktop backbuffer
        if (m_leftFbo && OpenGLProxy::glBlitFramebuffer) {
            OpenGLProxy::glBindFramebuffer(0x8CA8 /* GL_READ_FRAMEBUFFER */, m_leftFbo);
            OpenGLProxy::glBindFramebuffer(0x8CA9 /* GL_DRAW_FRAMEBUFFER */, 0);
            OpenGLProxy::glBlitFramebuffer(
                0, 0, m_eyeWidth, m_eyeHeight,
                0, 0, m_windowWidth, m_windowHeight,
                GL_COLOR_BUFFER_BIT, GL_LINEAR
            );
            OpenGLProxy::glBindFramebuffer(0x8D40, 0);
        }

        // Signal host that frame is ready
        m_ipcClient.SignalFrameReady();
    }
}

} // namespace RiddickVR
