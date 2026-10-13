#pragma once

#include "opengl_proxy.h"
#include "ipc_bridge.h"
#include <windows.h>
#include <GL/gl.h>
#include <d3d11.h>
#include <dxgi.h>

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
    void OnClear(GLbitfield mask);

    StereoPass GetCurrentPass() const { return m_currentPass; }
    void SetCurrentPass(StereoPass pass) { m_currentPass = pass; }

    GLuint GetActiveEyeFbo() const;
    GLuint GetLeftFbo() const { return m_leftFbo; }
    GLuint GetRightFbo() const { return m_rightFbo; }

    bool IsVREnabled() const { return m_vrActive; }
    const RiddickVRSharedState& GetLatestState() const { return m_sharedState; }

    float GetHudConvergenceOffset() const { return m_hudConvergenceOffset; }
    void SetHudConvergenceOffset(float offset) { m_hudConvergenceOffset = offset; }

    bool HasHardwareInterop() const { return m_dxInteropActive; }

private:
    StereoRenderer() = default;
    ~StereoRenderer() = default;

    bool CreateEyeBuffers(uint32_t width, uint32_t height);
    void DestroyEyeBuffers();

    bool InitD3D11Interop(uint32_t width, uint32_t height);
    void ReleaseD3D11Interop();
    void LockSharedTextures();
    void UnlockSharedTextures();

    StereoPass m_currentPass = StereoPass::Mono;
    bool m_vrActive = false;
    bool m_initialized = false;
    bool m_dxInteropActive = false;
    bool m_texturesLocked = false;

    // Eye Framebuffers & Textures
    GLuint m_leftFbo = 0;
    GLuint m_leftColorTex = 0;
    GLuint m_leftDepthRbo = 0;

    GLuint m_rightFbo = 0;
    GLuint m_rightColorTex = 0;
    GLuint m_rightDepthRbo = 0;

    // D3D11 Interop Resources
    ID3D11Device* m_d3d11Device = nullptr;
    ID3D11DeviceContext* m_d3d11Context = nullptr;
    HANDLE m_hDXDevice = nullptr;

    ID3D11Texture2D* m_d3d11LeftTex = nullptr;
    HANDLE m_hDXLeftTex = nullptr;
    HANDLE m_sharedHandleLeft = nullptr;

    ID3D11Texture2D* m_d3d11RightTex = nullptr;
    HANDLE m_hDXRightTex = nullptr;
    HANDLE m_sharedHandleRight = nullptr;

    uint32_t m_eyeWidth = 1920;
    uint32_t m_eyeHeight = 1080;

    // HUD virtual depth offset in normalized coordinates
    float m_hudConvergenceOffset = 0.015f; // ~1.5% screen offset = ~1.8m convergence

    // Window dimensions for spectator mirror
    GLsizei m_windowWidth = 1920;
    GLsizei m_windowHeight = 1080;

    IpcBridgeClient m_ipcClient;
    RiddickVRSharedState m_sharedState{};
};

} // namespace RiddickVR
