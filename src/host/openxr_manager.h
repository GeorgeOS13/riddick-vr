#pragma once

#include "riddick_vr_protocol.h"
#include "ipc_bridge.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <string>

namespace RiddickVR {

class OpenXRManager {
public:
    OpenXRManager();
    ~OpenXRManager();

    bool Initialize();
    void Shutdown();

    void RunLoop(IpcBridgeHost& ipcHost);
    void RequestExit() { m_running = false; }

    bool IsSessionActive() const { return m_sessionActive; }

private:
    bool PollEvents();
    void UpdateTracking(RiddickVRSharedState& state);
    void RenderFrame(IpcBridgeHost& ipcHost);

    bool EnsureSharedTextures(uint64_t leftHandle, uint64_t rightHandle);
    void ReleaseSharedTextures();

    bool m_initialized = false;
    bool m_running = false;
    bool m_sessionActive = false;
    uint32_t m_frameIndex = 0;

    // D3D11 Host Device
    ID3D11Device* m_d3d11Device = nullptr;
    ID3D11DeviceContext* m_d3d11Context = nullptr;

    // Opened shared textures from Hook process
    uint64_t m_currentLeftHandle = 0;
    uint64_t m_currentRightHandle = 0;
    ID3D11Texture2D* m_sharedLeftTex = nullptr;
    ID3D11Texture2D* m_sharedRightTex = nullptr;

    // Headset display configuration
    uint32_t m_recommendedWidth = 2048;
    uint32_t m_recommendedHeight = 2048;
};

} // namespace RiddickVR
