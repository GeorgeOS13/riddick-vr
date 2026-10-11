#pragma once

#include "riddick_vr_protocol.h"
#include "ipc_bridge.h"
#include <windows.h>
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

    bool m_initialized = false;
    bool m_running = false;
    bool m_sessionActive = false;
    uint32_t m_frameIndex = 0;

    // Headset display configuration
    uint32_t m_recommendedWidth = 2048;
    uint32_t m_recommendedHeight = 2048;
};

} // namespace RiddickVR
