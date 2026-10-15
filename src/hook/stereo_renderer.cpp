#include "stereo_renderer.h"
#include "camera_hook.h"
#include "vr_hand_renderer.h"
#include "logging.h"
#include <tlhelp32.h>
#include <algorithm>
#include <string>
#include <vector>

namespace RiddickVR {

static bool IsHostRunning() {
    bool running = false;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe{};
        pe.dwSize = sizeof(pe);
        if (Process32First(snapshot, &pe)) {
            do {
                if (_stricmp(pe.szExeFile, "riddickvr-host.exe") == 0) {
                    running = true;
                    break;
                }
            } while (Process32Next(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }
    return running;
}

static bool SpawnHostProcess() {
    static ULONGLONG s_lastSpawnAttempt = 0;
    ULONGLONG now = GetTickCount64();
    if (now - s_lastSpawnAttempt < 5000) {
        return false;
    }
    s_lastSpawnAttempt = now;

    if (IsHostRunning()) {
        VR_LOG_INFO("riddickvr-host.exe is already running in background.");
        return true;
    }

    // GH #26: never hardcode a developer machine path. Resolution order:
    // 1. RIDDICKVR_HOST env override, 2. hook DLL directory, 3. game EXE dir.
    std::string chosenHost = "";
    char envHost[MAX_PATH] = { 0 };
    if (GetEnvironmentVariableA("RIDDICKVR_HOST", envHost, MAX_PATH) > 0 &&
        GetFileAttributesA(envHost) != INVALID_FILE_ATTRIBUTES) {
        chosenHost = envHost;
    }

    auto DirOf = [](const char* path) {
        std::string dir = path;
        size_t lastSlash = dir.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            dir = dir.substr(0, lastSlash);
        }
        return dir;
    };

    if (chosenHost.empty()) {
        // Hook DLL directory (this code runs inside opengl32.dll).
        HMODULE hSelf = nullptr;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(&SpawnHostProcess), &hSelf) && hSelf) {
            char dllPath[MAX_PATH] = { 0 };
            if (GetModuleFileNameA(hSelf, dllPath, MAX_PATH) > 0) {
                std::string candidate = DirOf(dllPath) + "\\riddickvr-host.exe";
                if (GetFileAttributesA(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    chosenHost = candidate;
                }
            }
        }
    }

    std::string gameDir = "";
    if (chosenHost.empty()) {
        char modulePath[MAX_PATH] = { 0 };
        GetModuleFileNameA(NULL, modulePath, MAX_PATH);
        gameDir = DirOf(modulePath);
        std::string candidate = gameDir + "\\riddickvr-host.exe";
        if (GetFileAttributesA(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
            chosenHost = candidate;
        }
    }

    if (chosenHost.empty()) {
        VR_LOG_WARN("Could not locate riddickvr-host.exe next to the hook DLL or game EXE (RIDDICKVR_HOST override also unset).");
        return false;
    }

    VR_LOG_INFO("Auto-launching OpenXR Host Companion: %s", chosenHost.c_str());

    std::string hostWorkingDir = chosenHost.substr(0, chosenHost.find_last_of("\\/"));

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::string cmdLine = "\"" + chosenHost + "\" --auto-exit-with-game";
    std::vector<char> cmdLineBuf(cmdLine.begin(), cmdLine.end());
    cmdLineBuf.push_back('\0');

    // GH #26: CREATE_NO_WINDOW — never pop a console over the game.
    BOOL success = CreateProcessA(
        nullptr,
        cmdLineBuf.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        hostWorkingDir.c_str(),
        &si,
        &pi
    );

    if (success) {
        VR_LOG_INFO("Successfully launched riddickvr-host.exe (PID: %u)", pi.dwProcessId);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    } else {
        VR_LOG_WARN("Failed to launch riddickvr-host.exe (Error: %u)", GetLastError());
        return false;
    }
}

StereoRenderer& StereoRenderer::Instance() {
    static StereoRenderer s_instance;
    return s_instance;
}

bool StereoRenderer::Initialize(HDC hdc, HGLRC hglrc) {
    if (m_initialized) return true;

    VR_LOG_INFO("Initializing StereoRenderer for OpenGL context %p...", hglrc);

    // GH #29: bounded ~200ms sync budget on the render thread. One quick
    // connect attempt; on failure, fire-and-forget an async host spawn
    // (SpawnHostProcess is internally 5s-throttled) plus a single retry, then
    // fall back to Mono standby. The old 25x100ms loop blocked ~2.5s.
    // Late connection is retried from OnSwapBuffers without blocking.
    if (!m_ipcClient.Connect(100)) {
        VR_LOG_INFO("RiddickVRHost not detected. Attempting auto-spawn...");
        SpawnHostProcess();
        m_ipcClient.Connect(100); // single retry; Mono standby on failure
    }

    if (m_ipcClient.IsConnected()) {
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

    // GH #30: propagate eye-FBO failure — stay Mono instead of presenting
    // half-built stereo buffers.
    if (!CreateEyeBuffers(m_eyeWidth, m_eyeHeight)) {
        VR_LOG_ERROR("Eye framebuffer setup failed; staying in Mono standby.");
        m_vrActive = false;
        m_currentPass = StereoPass::Mono;
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

    // GH #30: require COMPLETE on both eyes; a half-built stereo pair must
    // fail closed (caller drops to Mono) rather than present garbage.
    if (leftStatus != 0x8CD5 /* GL_FRAMEBUFFER_COMPLETE */ ||
        rightStatus != 0x8CD5) {
        VR_LOG_ERROR("Eye FBO setup failed (left=0x%X right=0x%X); discarding buffers.",
            leftStatus, rightStatus);
        DestroyEyeBuffers();
        return false;
    }

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
    // Capture the primary game viewport, including pillarbox/letterbox active regions
    if (width >= 640 && height >= 480) {
        m_viewportX = x;
        m_viewportY = y;
        m_viewportWidth = width;
        m_viewportHeight = height;
        if (x == 0 && y == 0) {
            m_windowWidth = width;
            m_windowHeight = height;
        }
    }
}

void StereoRenderer::OnSwapBuffers(HDC hdc) {
    if (!m_initialized) return;

    static uint32_t s_swapCount = 0;
    static ULONGLONG s_lastFpsTime = 0;
    static uint32_t s_fpsFrameCount = 0;

    s_swapCount++;
    s_fpsFrameCount++;

    ULONGLONG now = GetTickCount64();
    if (now - s_lastFpsTime >= 5000) {
        float fps = s_fpsFrameCount / ((now - s_lastFpsTime) / 1000.0f);
        VR_LOG_INFO("OnSwapBuffers: Frame %u (%.1f FPS, HDC: %p)", s_swapCount, fps, hdc);
        s_lastFpsTime = now;
        s_fpsFrameCount = 0;
    }

    // Determine current window size from HDC if not already set or resized
    HWND hwnd = WindowFromDC(hdc);
    if (hwnd) {
        CameraHook::Instance().SetGameWindow(hwnd);
        RECT rc;
        if (GetClientRect(hwnd, &rc)) {
            if (rc.right > rc.left && rc.bottom > rc.top) {
                m_windowWidth = rc.right - rc.left;
                m_windowHeight = rc.bottom - rc.top;
            }
        }
    }

    // Retry connecting to host if not yet connected
    if (!m_ipcClient.IsConnected()) {
        static ULONGLONG s_lastRetry = 0;
        if (now - s_lastRetry > 3000) {
            s_lastRetry = now;
            if (m_ipcClient.Connect(100)) {
                VR_LOG_INFO("Late connected to RiddickVRHost!");
                m_vrActive = true;
            } else {
                SpawnHostProcess();
            }
        }
    }

    if (m_ipcClient.IsConnected()) {
        // Read updated tracking state from host
        if (m_ipcClient.ReadState(m_sharedState)) {
            m_vrActive = m_sharedState.vrActive;
            CameraHook::Instance().Update(m_sharedState);

            // Dynamically resize eye framebuffers if OpenXR headset requested specific dimensions
            if (m_sharedState.renderWidth > 0 && m_sharedState.renderHeight > 0 &&
                (m_sharedState.renderWidth != m_eyeWidth || m_sharedState.renderHeight != m_eyeHeight)) {
                VR_LOG_INFO("Resizing Stereo Eye Framebuffers to match OpenXR headset (%ux%u -> %ux%u)",
                    m_eyeWidth, m_eyeHeight, m_sharedState.renderWidth, m_sharedState.renderHeight);
                CreateEyeBuffers(m_sharedState.renderWidth, m_sharedState.renderHeight);
            }
        }

        // 1. Lock textures for OpenGL drawing
        LockSharedTextures();

        // 2. Capture the game's rendered backbuffer and blit into Left & Right eye FBOs
        if (OpenGLProxy::real_glBindFramebuffer && OpenGLProxy::glBlitFramebuffer) {
            GLint srcX = 0;
            GLint srcY = 0;
            GLint srcW = (m_windowWidth > 0) ? m_windowWidth : 1920;
            GLint srcH = (m_windowHeight > 0) ? m_windowHeight : 1080;

            if (m_viewportWidth >= 640 && m_viewportHeight >= 480) {
                srcX = m_viewportX;
                srcY = m_viewportY;
                srcW = m_viewportWidth;
                srcH = m_viewportHeight;
            }

            GLint dstX = 0;
            GLint dstY = 0;
            GLint dstW = m_eyeWidth;
            GLint dstH = m_eyeHeight;

            // GH #12: monoscopic capture has no IPD parallax (both eyes blit
            // the same backbuffer). Apply a small half-IPD horizontal shift per
            // eye as a convergence cue. This does NOT synthesize true parallax
            // (occlusion/disparity need an engine-level dual-view render).
            // TODO: picks/crosshair at vrActive depth via CameraHook-projected
            // dual frustum blits once the engine exposes per-eye viewports.
            // Desktop mirror is untouched: only eye-FBO source rects shift.
            GLint halfIpdPx = (std::max)(1, srcW / 240);
            GLint leftSrcX0 = srcX - halfIpdPx;
            GLint leftSrcX1 = srcX + srcW - halfIpdPx;
            GLint rightSrcX0 = srcX + halfIpdPx;
            GLint rightSrcX1 = srcX + srcW + halfIpdPx;

            // Read from default backbuffer 0
            OpenGLProxy::real_glBindFramebuffer(0x8CA8 /* GL_READ_FRAMEBUFFER */, 0);
            if (OpenGLProxy::glReadBuffer) {
                OpenGLProxy::glReadBuffer(0x0405 /* GL_BACK */);
            }

            // Blit into Left Eye FBO (vertically flipped: srcY goes from srcH down to 0)
            // Fixes: OpenGL bottom-left origin to Direct3D 11 / OpenXR top-left origin!
            // In gameplay mode (dstX=0, dstY=0, dstW=m_eyeWidth, dstH=m_eyeHeight),
            // this fills the entire buffer with 100% immersion and ZERO black bars.
            OpenGLProxy::real_glBindFramebuffer(0x8CA9 /* GL_DRAW_FRAMEBUFFER */, m_leftFbo);
            OpenGLProxy::glBlitFramebuffer(
                leftSrcX0, srcY + srcH, leftSrcX1, srcY,
                dstX, dstY, dstX + dstW, dstY + dstH,
                GL_COLOR_BUFFER_BIT, GL_LINEAR
            );

            // Render always-visible 3D VR tactical hands into Left Eye buffer
            VRHandRenderer::Instance().RenderHands(m_sharedState, true, m_eyeWidth, m_eyeHeight);

            // Blit into Right Eye FBO (vertically flipped: srcY goes from srcH down to 0)
            OpenGLProxy::real_glBindFramebuffer(0x8CA9 /* GL_DRAW_FRAMEBUFFER */, m_rightFbo);
            OpenGLProxy::glBlitFramebuffer(
                rightSrcX0, srcY + srcH, rightSrcX1, srcY,
                dstX, dstY, dstX + dstW, dstY + dstH,
                GL_COLOR_BUFFER_BIT, GL_LINEAR
            );

            // Render always-visible 3D VR tactical hands into Right Eye buffer
            VRHandRenderer::Instance().RenderHands(m_sharedState, false, m_eyeWidth, m_eyeHeight);

            // Force alpha channel to fully opaque 1.0f on both eye buffers to eliminate
            // Virtual Desktop / VDXR green screen chroma keying and transparency flashes
            if (OpenGLProxy::glColorMask && OpenGLProxy::glClearColor && OpenGLProxy::real_glClearFunc) {
                OpenGLProxy::glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
                OpenGLProxy::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

                OpenGLProxy::real_glBindFramebuffer(0x8CA9 /* GL_DRAW_FRAMEBUFFER */, m_leftFbo);
                OpenGLProxy::real_glClearFunc(GL_COLOR_BUFFER_BIT);

                OpenGLProxy::real_glBindFramebuffer(0x8CA9 /* GL_DRAW_FRAMEBUFFER */, m_rightFbo);
                OpenGLProxy::real_glClearFunc(GL_COLOR_BUFFER_BIT);

                OpenGLProxy::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            }

            // Restore default framebuffer 0 for the game and spectator display
            OpenGLProxy::real_glBindFramebuffer(0x8D40 /* GL_FRAMEBUFFER */, 0);
        }

        // Flush OpenGL command queue so GPU work is committed before unlocking
        if (OpenGLProxy::glFlush) {
            OpenGLProxy::glFlush();
        }

        // 3. Unlock textures so D3D11 / OpenXR host can read them
        UnlockSharedTextures();

        // Flush D3D11 device context so GPU commands on the shared resource are committed
        // before the separate OpenXR host process accesses it
        if (m_d3d11Context) {
            m_d3d11Context->Flush();
        }

        // Re-assign persistent GPU shared texture handles
        if (m_dxInteropActive) {
            m_sharedState.leftEyeSharedHandle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_sharedHandleLeft));
            m_sharedState.rightEyeSharedHandle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_sharedHandleRight));
        }

        // Write client frame count and state
        m_ipcClient.WriteState(m_sharedState);

        // Signal host that frame is ready
        m_ipcClient.SignalFrameReady();

        // Alternate stereoscopic eye pass for the upcoming scene draw
        if (m_currentPass == StereoPass::LeftEye) {
            m_currentPass = StereoPass::RightEye;
        } else {
            m_currentPass = StereoPass::LeftEye;
        }
    }
}

GLuint StereoRenderer::GetActiveEyeFbo() const {
    if (!m_vrActive) return 0;
    return (m_currentPass == StereoPass::RightEye) ? m_rightFbo : m_leftFbo;
}

void StereoRenderer::OnClear(GLbitfield mask) {
    // Intentionally no-op: let the game engine clear its own framebuffers, shadow maps, and stencil buffers
}

} // namespace RiddickVR
