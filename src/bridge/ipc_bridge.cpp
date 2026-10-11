#include "ipc_bridge.h"
#include <iostream>

namespace RiddickVR {

// ============================================================================
// IpcBridgeHost Implementation (used by 64-bit RiddickVRHost.exe)
// ============================================================================

IpcBridgeHost::IpcBridgeHost() = default;

IpcBridgeHost::~IpcBridgeHost() {
    Shutdown();
}

bool IpcBridgeHost::Initialize() {
    if (m_initialized) return true;

    // Create or open the shared memory mapping
    m_hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(RiddickVRSharedState),
        RIDDICK_VR_IPC_SHARED_MEM_NAME
    );

    if (!m_hMapFile) {
        return false;
    }

    m_pSharedState = static_cast<RiddickVRSharedState*>(
        MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(RiddickVRSharedState))
    );

    if (!m_pSharedState) {
        CloseHandle(m_hMapFile);
        m_hMapFile = nullptr;
        return false;
    }

    // Initialize state
    ZeroMemory(m_pSharedState, sizeof(RiddickVRSharedState));
    m_pSharedState->protocolVersion = RIDDICK_VR_PROTOCOL_VERSION;
    m_pSharedState->vrActive = true;

    // Create synchronization events (manual reset = false)
    m_hFrameReadyEvent = CreateEventA(nullptr, FALSE, FALSE, RIDDICK_VR_IPC_FRAME_READY_EVENT);
    m_hFrameConsumedEvent = CreateEventA(nullptr, FALSE, FALSE, RIDDICK_VR_IPC_FRAME_CONSUMED_EVENT);
    m_hMutex = CreateMutexA(nullptr, FALSE, RIDDICK_VR_IPC_MUTEX_NAME);

    m_initialized = true;
    return true;
}

void IpcBridgeHost::Shutdown() {
    if (m_pSharedState) {
        m_pSharedState->vrActive = false;
        UnmapViewOfFile(m_pSharedState);
        m_pSharedState = nullptr;
    }

    if (m_hMapFile) {
        CloseHandle(m_hMapFile);
        m_hMapFile = nullptr;
    }

    if (m_hFrameReadyEvent) {
        CloseHandle(m_hFrameReadyEvent);
        m_hFrameReadyEvent = nullptr;
    }

    if (m_hFrameConsumedEvent) {
        CloseHandle(m_hFrameConsumedEvent);
        m_hFrameConsumedEvent = nullptr;
    }

    if (m_hMutex) {
        CloseHandle(m_hMutex);
        m_hMutex = nullptr;
    }

    m_initialized = false;
}

bool IpcBridgeHost::WriteState(const RiddickVRSharedState& state) {
    if (!m_initialized || !m_pSharedState) return false;

    if (WaitForSingleObject(m_hMutex, 50) == WAIT_OBJECT_0) {
        // Copy tracking, poses, inputs, and heartbeat
        m_pSharedState->frameIndex = state.frameIndex;
        m_pSharedState->hmdPose = state.hmdPose;
        m_pSharedState->leftEyeView = state.leftEyeView;
        m_pSharedState->leftEyeProjection = state.leftEyeProjection;
        m_pSharedState->rightEyeView = state.rightEyeView;
        m_pSharedState->rightEyeProjection = state.rightEyeProjection;
        m_pSharedState->leftController = state.leftController;
        m_pSharedState->rightController = state.rightController;
        m_pSharedState->renderWidth = state.renderWidth;
        m_pSharedState->renderHeight = state.renderHeight;
        m_pSharedState->vrActive = state.vrActive;
        m_pSharedState->shouldRecenter = state.shouldRecenter;
        m_pSharedState->heartbeatTimestamp = GetTickCount();

        ReleaseMutex(m_hMutex);
        return true;
    }
    return false;
}

bool IpcBridgeHost::ReadState(RiddickVRSharedState& state) {
    if (!m_initialized || !m_pSharedState) return false;

    if (WaitForSingleObject(m_hMutex, 50) == WAIT_OBJECT_0) {
        state = *m_pSharedState;
        ReleaseMutex(m_hMutex);
        return true;
    }
    return false;
}

bool IpcBridgeHost::WaitForFrameReady(uint32_t timeoutMs) {
    if (!m_hFrameReadyEvent) return false;
    return WaitForSingleObject(m_hFrameReadyEvent, timeoutMs) == WAIT_OBJECT_0;
}

void IpcBridgeHost::SignalFrameConsumed() {
    if (m_hFrameConsumedEvent) {
        SetEvent(m_hFrameConsumedEvent);
    }
}

bool IpcBridgeHost::IsClientConnected() const {
    if (!m_pSharedState) return false;
    return m_pSharedState->clientConnected;
}

// ============================================================================
// IpcBridgeClient Implementation (used by 32-bit opengl32.dll)
// ============================================================================

IpcBridgeClient::IpcBridgeClient() = default;

IpcBridgeClient::~IpcBridgeClient() {
    Disconnect();
}

bool IpcBridgeClient::Connect(uint32_t timeoutMs) {
    if (m_connected) return true;

    const DWORD startTime = GetTickCount();
    while (GetTickCount() - startTime < timeoutMs) {
        m_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, RIDDICK_VR_IPC_SHARED_MEM_NAME);
        if (m_hMapFile) break;
        Sleep(50);
    }

    if (!m_hMapFile) {
        return false;
    }

    m_pSharedState = static_cast<RiddickVRSharedState*>(
        MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(RiddickVRSharedState))
    );

    if (!m_pSharedState) {
        CloseHandle(m_hMapFile);
        m_hMapFile = nullptr;
        return false;
    }

    if (m_pSharedState->protocolVersion != RIDDICK_VR_PROTOCOL_VERSION) {
        Disconnect();
        return false;
    }

    m_hFrameReadyEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, RIDDICK_VR_IPC_FRAME_READY_EVENT);
    m_hFrameConsumedEvent = OpenEventA(SYNCHRONIZE, FALSE, RIDDICK_VR_IPC_FRAME_CONSUMED_EVENT);
    m_hMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, RIDDICK_VR_IPC_MUTEX_NAME);

    m_connected = true;
    if (m_hMutex && WaitForSingleObject(m_hMutex, 50) == WAIT_OBJECT_0) {
        m_pSharedState->clientConnected = true;
        ReleaseMutex(m_hMutex);
    }
    return true;
}

void IpcBridgeClient::Disconnect() {
    if (m_pSharedState) {
        if (m_hMutex && WaitForSingleObject(m_hMutex, 50) == WAIT_OBJECT_0) {
            m_pSharedState->clientConnected = false;
            ReleaseMutex(m_hMutex);
        }
        UnmapViewOfFile(m_pSharedState);
        m_pSharedState = nullptr;
    }

    if (m_hMapFile) {
        CloseHandle(m_hMapFile);
        m_hMapFile = nullptr;
    }

    if (m_hFrameReadyEvent) {
        CloseHandle(m_hFrameReadyEvent);
        m_hFrameReadyEvent = nullptr;
    }

    if (m_hFrameConsumedEvent) {
        CloseHandle(m_hFrameConsumedEvent);
        m_hFrameConsumedEvent = nullptr;
    }

    if (m_hMutex) {
        CloseHandle(m_hMutex);
        m_hMutex = nullptr;
    }

    m_connected = false;
}

bool IpcBridgeClient::ReadState(RiddickVRSharedState& state) {
    if (!m_connected || !m_pSharedState) return false;

    if (WaitForSingleObject(m_hMutex, 20) == WAIT_OBJECT_0) {
        state = *m_pSharedState;
        ReleaseMutex(m_hMutex);
        return true;
    }
    return false;
}

bool IpcBridgeClient::WriteState(const RiddickVRSharedState& state) {
    if (!m_connected || !m_pSharedState) return false;

    if (WaitForSingleObject(m_hMutex, 20) == WAIT_OBJECT_0) {
        m_pSharedState->clientConnected = true;
        m_pSharedState->clientFrameCount++;
        m_pSharedState->leftEyeSharedHandle = state.leftEyeSharedHandle;
        m_pSharedState->rightEyeSharedHandle = state.rightEyeSharedHandle;
        ReleaseMutex(m_hMutex);
        return true;
    }
    return false;
}

void IpcBridgeClient::SignalFrameReady() {
    if (m_hFrameReadyEvent) {
        SetEvent(m_hFrameReadyEvent);
    }
}

bool IpcBridgeClient::WaitForFrameConsumed(uint32_t timeoutMs) {
    if (!m_hFrameConsumedEvent) return false;
    return WaitForSingleObject(m_hFrameConsumedEvent, timeoutMs) == WAIT_OBJECT_0;
}

} // namespace RiddickVR
