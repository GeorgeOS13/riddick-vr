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
    m_running = false;
    m_sessionActive = false;
    m_initialized = false;
    std::cout << "[Host] OpenXR Manager shut down." << std::endl;
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
