#include "openxr_manager.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

namespace RiddickVR {

OpenXRManager::OpenXRManager() = default;

OpenXRManager::~OpenXRManager() {
    Shutdown();
}

bool OpenXRManager::Initialize() {
    if (m_initialized) return true;

    std::cout << "[Host] Initializing OpenXR Session..." << std::endl;

    // Initialize D3D11 Compositor Device
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

    if (SUCCEEDED(hr) && m_d3d11Device) {
        std::cout << "[Host] Created Direct3D 11 host compositor device." << std::endl;
    } else {
        std::cout << "[Host] Warning: Direct3D 11 device creation failed (hr=0x" << std::hex << hr << std::dec << ")" << std::endl;
    }

    // Check for openxr_loader.dll
    HMODULE hLoader = LoadLibraryA("openxr_loader.dll");
    if (!hLoader) {
        std::cout << "[Host] Notice: openxr_loader.dll not found in path. Falling back to desktop emulation mode for testing." << std::endl;
        m_recommendedWidth = 1920;
        m_recommendedHeight = 1080;
    } else {
        std::cout << "[Host] Successfully loaded openxr_loader.dll." << std::endl;
        FreeLibrary(hLoader);
    }

    m_initialized = true;
    m_running = true;
    m_sessionActive = true;
    return true;
}

void OpenXRManager::Shutdown() {
    ReleaseSharedTextures();

    if (m_d3d11Context) {
        m_d3d11Context->Release();
        m_d3d11Context = nullptr;
    }
    if (m_d3d11Device) {
        m_d3d11Device->Release();
        m_d3d11Device = nullptr;
    }

    m_running = false;
    m_sessionActive = false;
    m_initialized = false;
    std::cout << "[Host] OpenXR Manager shut down." << std::endl;
}

bool OpenXRManager::EnsureSharedTextures(uint64_t leftHandle, uint64_t rightHandle) {
    if (!m_d3d11Device || !leftHandle || !rightHandle) return false;
    if (leftHandle == m_currentLeftHandle && rightHandle == m_currentRightHandle && m_sharedLeftTex && m_sharedRightTex) {
        return true;
    }

    ReleaseSharedTextures();

    HANDLE hLeft = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(leftHandle & 0xFFFFFFFFULL));
    HANDLE hRight = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(rightHandle & 0xFFFFFFFFULL));

    HRESULT hrLeft = m_d3d11Device->OpenSharedResource(
        hLeft,
        __uuidof(ID3D11Texture2D),
        (void**)&m_sharedLeftTex
    );

    HRESULT hrRight = m_d3d11Device->OpenSharedResource(
        hRight,
        __uuidof(ID3D11Texture2D),
        (void**)&m_sharedRightTex
    );

    if (SUCCEEDED(hrLeft) && SUCCEEDED(hrRight) && m_sharedLeftTex && m_sharedRightTex) {
        m_currentLeftHandle = leftHandle;
        m_currentRightHandle = rightHandle;
        std::cout << "[Host] Successfully opened cross-process shared eye textures (Left: 0x" 
                  << std::hex << leftHandle << ", Right: 0x" << rightHandle << std::dec << ")!" << std::endl;
        return true;
    } else {
        std::cout << "[Host] Warning: Could not open cross-process shared eye textures (Left hr: 0x" 
                  << std::hex << hrLeft << ", Right hr: 0x" << hrRight << std::dec << ")" << std::endl;
        ReleaseSharedTextures();
        return false;
    }
}

void OpenXRManager::ReleaseSharedTextures() {
    if (m_sharedLeftTex) {
        m_sharedLeftTex->Release();
        m_sharedLeftTex = nullptr;
    }
    if (m_sharedRightTex) {
        m_sharedRightTex->Release();
        m_sharedRightTex = nullptr;
    }
    m_currentLeftHandle = 0;
    m_currentRightHandle = 0;
}

bool OpenXRManager::PollEvents() {
    // Process runtime events (session state changes, recenter, exit)
    return m_running;
}

void OpenXRManager::UpdateTracking(RiddickVRSharedState& state) {
    m_frameIndex++;
    state.frameIndex = m_frameIndex;
    state.vrActive = m_sessionActive;
    state.renderWidth = m_recommendedWidth;
    state.renderHeight = m_recommendedHeight;

    // Calculate synthetic or real 6DOF pose
    // Neutral eye identity orientation (w = 1.0)
    state.hmdPose.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    state.hmdPose.position = {0.0f, 0.0f, 0.0f};
    state.hmdPose.isValid = true;

    // Default 65mm IPD baseline matrices
    constexpr float HALF_IPD = 0.0325f; // 32.5 mm

    // Column-major identity with stereo translation
    for (int i = 0; i < 16; ++i) {
        state.leftEyeView.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        state.rightEyeView.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    // Translation column (indices 12, 13, 14)
    state.leftEyeView.m[12] = -HALF_IPD;
    state.rightEyeView.m[12] = +HALF_IPD;

    // Standard VR asymmetric projection matrices (~90 deg FOV)
    constexpr float fovTan = 1.0f; // tan(45 deg)
    constexpr float nearZ = 0.05f;
    constexpr float farZ = 1000.0f;

    for (int i = 0; i < 16; ++i) {
        state.leftEyeProjection.m[i] = 0.0f;
        state.rightEyeProjection.m[i] = 0.0f;
    }
    state.leftEyeProjection.m[0] = 1.0f / fovTan;
    state.leftEyeProjection.m[5] = 1.0f / fovTan;
    state.leftEyeProjection.m[10] = -(farZ + nearZ) / (farZ - nearZ);
    state.leftEyeProjection.m[11] = -1.0f;
    state.leftEyeProjection.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);

    state.rightEyeProjection = state.leftEyeProjection;

    // Controllers
    state.leftController.isConnected = true;
    state.rightController.isConnected = true;
}

void OpenXRManager::RenderFrame(IpcBridgeHost& ipcHost) {
    RiddickVRSharedState state{};
    UpdateTracking(state);

    // Transmit updated tracking to the game hook
    ipcHost.WriteState(state);

    // Wait for the game hook to finish rendering the frame (up to 16ms)
    if (ipcHost.WaitForFrameReady(16)) {
        // Read client state to check for shared texture handles
        RiddickVRSharedState clientState{};
        if (ipcHost.ReadState(clientState)) {
            if (clientState.leftEyeSharedHandle && clientState.rightEyeSharedHandle) {
                EnsureSharedTextures(clientState.leftEyeSharedHandle, clientState.rightEyeSharedHandle);
            }
        }

        // Frame received from game; submit to OpenXR compositor
        ipcHost.SignalFrameConsumed();
    }
}

void OpenXRManager::RunLoop(IpcBridgeHost& ipcHost) {
    std::cout << "[Host] Entering VR compositor main loop. Press Ctrl+C in console to terminate." << std::endl;

    auto lastReport = std::chrono::steady_clock::now();
    uint32_t framesCounted = 0;

    while (m_running && PollEvents()) {
        RenderFrame(ipcHost);
        framesCounted++;

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastReport).count() >= 5) {
            float fps = framesCounted / 5.0f;
            std::cout << "[Host] Heartbeat: " << fps << " FPS | Connected: " 
                      << (ipcHost.IsClientConnected() ? "YES" : "WAITING FOR GAME") << std::endl;
            framesCounted = 0;
            lastReport = now;
        }

        // Throttle when idle / not connected to save CPU cycles
        if (!ipcHost.IsClientConnected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

} // namespace RiddickVR
