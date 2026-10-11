#pragma once

#include "riddick_vr_protocol.h"
#include <windows.h>
#include <string>

namespace RiddickVR {

class IpcBridgeHost {
public:
    IpcBridgeHost();
    ~IpcBridgeHost();

    bool Initialize();
    void Shutdown();

    bool WriteState(const RiddickVRSharedState& state);
    bool ReadState(RiddickVRSharedState& state);

    bool WaitForFrameReady(uint32_t timeoutMs = 100);
    void SignalFrameConsumed();

    bool IsClientConnected() const;

private:
    HANDLE m_hMapFile = nullptr;
    RiddickVRSharedState* m_pSharedState = nullptr;
    HANDLE m_hFrameReadyEvent = nullptr;
    HANDLE m_hFrameConsumedEvent = nullptr;
    HANDLE m_hMutex = nullptr;
    bool m_initialized = false;
};

class IpcBridgeClient {
public:
    IpcBridgeClient();
    ~IpcBridgeClient();

    bool Connect(uint32_t timeoutMs = 2000);
    void Disconnect();

    bool ReadState(RiddickVRSharedState& state);
    bool WriteState(const RiddickVRSharedState& state);

    void SignalFrameReady();
    bool WaitForFrameConsumed(uint32_t timeoutMs = 50);

    bool IsConnected() const { return m_connected; }

private:
    HANDLE m_hMapFile = nullptr;
    RiddickVRSharedState* m_pSharedState = nullptr;
    HANDLE m_hFrameReadyEvent = nullptr;
    HANDLE m_hFrameConsumedEvent = nullptr;
    HANDLE m_hMutex = nullptr;
    bool m_connected = false;
};

} // namespace RiddickVR
