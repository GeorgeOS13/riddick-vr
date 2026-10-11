#pragma once

#include "opengl_proxy.h"
#include "ipc_bridge.h"
#include <windows.h>
#include <GL/gl.h>

namespace RiddickVR {

enum class StereoPass {
    Mono,
    LeftEye,
    RightEye
};

class StereoRenderer {
public:
    static StereoRenderer& Instance();

    bool Initialize(HDC hdc, HGLRC hglrc);
    void Shutdown();

    void OnViewport(GLint x, GLint y, GLsizei width, GLsizei height);
    void OnSwapBuffers(HDC hdc);

    StereoPass GetCurrentPass() const { return m_currentPass; }
    void SetCurrentPass(StereoPass pass) { m_currentPass = pass; }

    bool IsVREnabled() const { return m_vrActive; }
    const RiddickVRSharedState& GetLatestState() const { return m_sharedState; }

private:
    StereoRenderer() = default;
    ~StereoRenderer() = default;

    bool CreateEyeBuffers(uint32_t width, uint32_t height);
    void DestroyEyeBuffers();

    StereoPass m_currentPass = StereoPass::Mono;
    bool m_vrActive = false;
    bool m_initialized = false;

    // Eye Framebuffers & Textures
    GLuint m_leftFbo = 0;
    GLuint m_leftColorTex = 0;
    GLuint m_leftDepthRbo = 0;

    GLuint m_rightFbo = 0;
    GLuint m_rightColorTex = 0;
    GLuint m_rightDepthRbo = 0;

    uint32_t m_eyeWidth = 1920;
    uint32_t m_eyeHeight = 1080;

    // Window dimensions for spectator mirror
    GLsizei m_windowWidth = 1920;
    GLsizei m_windowHeight = 1080;

    IpcBridgeClient m_ipcClient;
    RiddickVRSharedState m_sharedState{};
};

} // namespace RiddickVR
