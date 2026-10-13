#include "stereo_renderer.h"
#include "camera_hook.h"
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
        m_currentPass = StereoPass::LeftEye;
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
        m_currentPass = StereoPass::Mono;
    }

    CreateEyeBuffers(m_eyeWidth, m_eyeHeight);

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

    // --- Left Eye FBO ---
    OpenGLProxy::glGenFramebuffers(1, &m_leftFbo);
    OpenGLProxy::glGenTextures(1, &m_leftColorTex);
    OpenGLProxy::glBindTexture(GL_TEXTURE_2D, m_leftColorTex);
    OpenGLProxy::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    OpenGLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    OpenGLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    OpenGLProxy::glBindFramebuffer(0x8D40 /* GL_FRAMEBUFFER */, m_leftFbo);
    OpenGLProxy::glFramebufferTexture2D(0x8D40, 0x8CE0 /* GL_COLOR_ATTACHMENT0 */, GL_TEXTURE_2D, m_leftColorTex, 0);

    if (OpenGLProxy::glGenRenderbuffers) {
        OpenGLProxy::glGenRenderbuffers(1, &m_leftDepthRbo);
        OpenGLProxy::glBindRenderbuffer(0x8D41 /* GL_RENDERBUFFER */, m_leftDepthRbo);
        OpenGLProxy::glRenderbufferStorage(0x8D41, 0x88F0 /* GL_DEPTH24_STENCIL8 */, width, height);
        OpenGLProxy::glFramebufferRenderbuffer(0x8D40, 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */, 0x8D41, m_leftDepthRbo);
    }

    GLenum leftStatus = OpenGLProxy::glCheckFramebufferStatus ? OpenGLProxy::glCheckFramebufferStatus(0x8D40) : 0x8CD5;
    if (leftStatus != 0x8CD5 /* GL_FRAMEBUFFER_COMPLETE */) {
        VR_LOG_WARN("Left Eye FBO incomplete: status 0x%X", leftStatus);
    }

    // --- Right Eye FBO ---
    OpenGLProxy::glGenFramebuffers(1, &m_rightFbo);
    OpenGLProxy::glGenTextures(1, &m_rightColorTex);
    OpenGLProxy::glBindTexture(GL_TEXTURE_2D, m_rightColorTex);
    OpenGLProxy::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    OpenGLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    OpenGLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    OpenGLProxy::glBindFramebuffer(0x8D40, m_rightFbo);
    OpenGLProxy::glFramebufferTexture2D(0x8D40, 0x8CE0, GL_TEXTURE_2D, m_rightColorTex, 0);

    if (OpenGLProxy::glGenRenderbuffers) {
        OpenGLProxy::glGenRenderbuffers(1, &m_rightDepthRbo);
        OpenGLProxy::glBindRenderbuffer(0x8D41, m_rightDepthRbo);
        OpenGLProxy::glRenderbufferStorage(0x8D41, 0x88F0, width, height);
        OpenGLProxy::glFramebufferRenderbuffer(0x8D40, 0x821A, 0x8D41, m_rightDepthRbo);
    }

    GLenum rightStatus = OpenGLProxy::glCheckFramebufferStatus ? OpenGLProxy::glCheckFramebufferStatus(0x8D40) : 0x8CD5;
    if (rightStatus != 0x8CD5) {
        VR_LOG_WARN("Right Eye FBO incomplete: status 0x%X", rightStatus);
    }

    // Unbind back to default framebuffer
    OpenGLProxy::glBindFramebuffer(0x8D40, 0);
    OpenGLProxy::glBindTexture(GL_TEXTURE_2D, 0);

    // Initialize hardware texture sharing if supported
    InitD3D11Interop(width, height);
    LockSharedTextures();

    VR_LOG_INFO("Stereo Eye Framebuffers successfully created (%ux%u, LeftFBO: %u, RightFBO: %u)",
        width, height, m_leftFbo, m_rightFbo);
    return true;
}

void StereoRenderer::DestroyEyeBuffers() {
    ReleaseD3D11Interop();

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

    if (OpenGLProxy::glDeleteRenderbuffers) {
        if (m_leftDepthRbo) {
            OpenGLProxy::glDeleteRenderbuffers(1, &m_leftDepthRbo);
            m_leftDepthRbo = 0;
        }
        if (m_rightDepthRbo) {
            OpenGLProxy::glDeleteRenderbuffers(1, &m_rightDepthRbo);
            m_rightDepthRbo = 0;
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

bool StereoRenderer::InitD3D11Interop(uint32_t width, uint32_t height) {
    if (!OpenGLProxy::HasDXInteropSupport()) {
        VR_LOG_INFO("Hardware Interop: WGL_NV_DX_interop not available; using standard FBO pipeline.");
        return false;
    }

    ReleaseD3D11Interop();

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevels,
        2,
        D3D11_SDK_VERSION,
        &m_d3d11Device,
        &featureLevel,
        &m_d3d11Context
    );

    if (FAILED(hr) || !m_d3d11Device) {
        VR_LOG_WARN("Hardware Interop: D3D11CreateDevice failed with hr=0x%08X", hr);
        return false;
    }

    m_hDXDevice = OpenGLProxy::wglDXOpenDeviceNV(m_d3d11Device);
    if (!m_hDXDevice) {
        VR_LOG_WARN("Hardware Interop: wglDXOpenDeviceNV failed!");
        ReleaseD3D11Interop();
        return false;
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    // Create Left Eye Shared D3D11 Texture
    hr = m_d3d11Device->CreateTexture2D(&desc, nullptr, &m_d3d11LeftTex);
    if (SUCCEEDED(hr) && m_d3d11LeftTex) {
        IDXGIResource* pResource = nullptr;
        if (SUCCEEDED(m_d3d11LeftTex->QueryInterface(__uuidof(IDXGIResource), (void**)&pResource))) {
            pResource->GetSharedHandle(&m_sharedHandleLeft);
            pResource->Release();
        }
    }

    // Create Right Eye Shared D3D11 Texture
    hr = m_d3d11Device->CreateTexture2D(&desc, nullptr, &m_d3d11RightTex);
    if (SUCCEEDED(hr) && m_d3d11RightTex) {
        IDXGIResource* pResource = nullptr;
        if (SUCCEEDED(m_d3d11RightTex->QueryInterface(__uuidof(IDXGIResource), (void**)&pResource))) {
            pResource->GetSharedHandle(&m_sharedHandleRight);
            pResource->Release();
        }
    }

    if (!m_sharedHandleLeft || !m_sharedHandleRight) {
        VR_LOG_WARN("Hardware Interop: Failed to retrieve DXGI shared resource handles.");
        ReleaseD3D11Interop();
        return false;
    }

    // Register with OpenGL textures
    m_hDXLeftTex = OpenGLProxy::wglDXRegisterObjectNV(
        m_hDXDevice, m_d3d11LeftTex, m_leftColorTex, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV
    );
    m_hDXRightTex = OpenGLProxy::wglDXRegisterObjectNV(
        m_hDXDevice, m_d3d11RightTex, m_rightColorTex, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV
    );

    if (!m_hDXLeftTex || !m_hDXRightTex) {
        VR_LOG_WARN("Hardware Interop: wglDXRegisterObjectNV failed!");
        ReleaseD3D11Interop();
        return false;
    }

    m_dxInteropActive = true;
    m_sharedState.leftEyeSharedHandle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_sharedHandleLeft));
    m_sharedState.rightEyeSharedHandle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_sharedHandleRight));

    if (m_ipcClient.IsConnected()) {
        m_ipcClient.WriteState(m_sharedState);
    }

    VR_LOG_INFO("Hardware Interop: SUCCESS! Zero-copy GPU texture sharing ACTIVE (Left: 0x%p, Right: 0x%p)",
        m_sharedHandleLeft, m_sharedHandleRight);
    return true;
}

void StereoRenderer::ReleaseD3D11Interop() {
    UnlockSharedTextures();

    if (m_hDXDevice) {
        if (m_hDXLeftTex && OpenGLProxy::wglDXUnregisterObjectNV) {
            OpenGLProxy::wglDXUnregisterObjectNV(m_hDXDevice, m_hDXLeftTex);
            m_hDXLeftTex = nullptr;
        }
        if (m_hDXRightTex && OpenGLProxy::wglDXUnregisterObjectNV) {
            OpenGLProxy::wglDXUnregisterObjectNV(m_hDXDevice, m_hDXRightTex);
            m_hDXRightTex = nullptr;
        }
        if (OpenGLProxy::wglDXCloseDeviceNV) {
            OpenGLProxy::wglDXCloseDeviceNV(m_hDXDevice);
        }
        m_hDXDevice = nullptr;
    }

    if (m_d3d11LeftTex) {
        m_d3d11LeftTex->Release();
        m_d3d11LeftTex = nullptr;
    }
    if (m_d3d11RightTex) {
        m_d3d11RightTex->Release();
        m_d3d11RightTex = nullptr;
    }
    if (m_d3d11Context) {
        m_d3d11Context->Release();
        m_d3d11Context = nullptr;
    }
    if (m_d3d11Device) {
        m_d3d11Device->Release();
        m_d3d11Device = nullptr;
    }

    m_sharedHandleLeft = nullptr;
    m_sharedHandleRight = nullptr;
    m_dxInteropActive = false;
}

void StereoRenderer::LockSharedTextures() {
    if (!m_dxInteropActive || m_texturesLocked || !m_hDXDevice || !OpenGLProxy::wglDXLockObjectsNV) return;

    HANDLE objects[2] = { m_hDXLeftTex, m_hDXRightTex };
    if (OpenGLProxy::wglDXLockObjectsNV(m_hDXDevice, 2, objects)) {
        m_texturesLocked = true;
    }
}

void StereoRenderer::UnlockSharedTextures() {
    if (!m_dxInteropActive || !m_texturesLocked || !m_hDXDevice || !OpenGLProxy::wglDXUnlockObjectsNV) return;

    HANDLE objects[2] = { m_hDXLeftTex, m_hDXRightTex };
    if (OpenGLProxy::wglDXUnlockObjectsNV(m_hDXDevice, 2, objects)) {
        m_texturesLocked = false;
    }
}

void StereoRenderer::OnViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    m_windowWidth = width;
    m_windowHeight = height;
}

void StereoRenderer::OnSwapBuffers(HDC hdc) {
    if (!m_initialized) return;

    static uint32_t s_swapCount = 0;
    static DWORD s_lastFpsTime = 0;
    static uint32_t s_fpsFrameCount = 0;

    s_swapCount++;
    s_fpsFrameCount++;

    DWORD now = GetTickCount();
    if (now - s_lastFpsTime >= 5000) {
        float fps = s_fpsFrameCount / ((now - s_lastFpsTime) / 1000.0f);
        VR_LOG_INFO("OnSwapBuffers: Frame %u (%.1f FPS, HDC: %p)", s_swapCount, fps, hdc);
        s_lastFpsTime = now;
        s_fpsFrameCount = 0;
    }

    // Retry connecting to host if not yet connected
    if (!m_ipcClient.IsConnected()) {
        static DWORD s_lastRetry = 0;
        if (now - s_lastRetry > 3000) {
            s_lastRetry = now;
            if (m_ipcClient.Connect(100)) {
                VR_LOG_INFO("Late connected to RiddickVRHost!");
                m_vrActive = true;
            }
        }
    }

    if (m_ipcClient.IsConnected()) {
        // Unlock GPU interop textures before notifying host OpenXR compositor
        UnlockSharedTextures();

        // Read updated tracking state for next frame
        if (m_ipcClient.ReadState(m_sharedState)) {
            m_vrActive = m_sharedState.vrActive;
            CameraHook::Instance().Update(m_sharedState);
        }

        // Re-assign persistent GPU shared texture handles
        if (m_dxInteropActive) {
            m_sharedState.leftEyeSharedHandle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_sharedHandleLeft));
            m_sharedState.rightEyeSharedHandle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_sharedHandleRight));
        }

        // Write client frame count and state
        m_ipcClient.WriteState(m_sharedState);

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

        // Alternate stereoscopic eye pass for the upcoming scene draw
        if (m_currentPass == StereoPass::LeftEye) {
            m_currentPass = StereoPass::RightEye;
        } else {
            m_currentPass = StereoPass::LeftEye;
        }

        // Re-lock shared textures for the upcoming frame
        LockSharedTextures();
    }
}

GLuint StereoRenderer::GetActiveEyeFbo() const {
    if (!m_vrActive) return 0;
    return (m_currentPass == StereoPass::RightEye) ? m_rightFbo : m_leftFbo;
}

void StereoRenderer::OnClear(GLbitfield mask) {
    if (!m_vrActive) return;
    GLuint activeFbo = GetActiveEyeFbo();
    if (activeFbo && OpenGLProxy::glBindFramebuffer) {
        OpenGLProxy::glBindFramebuffer(0x8D40 /* GL_FRAMEBUFFER */, activeFbo);
    }
}

} // namespace RiddickVR
