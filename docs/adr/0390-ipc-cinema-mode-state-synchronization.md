# ADR 0390: Real-Time IPC Cinema Mode State Synchronization

## Context
The VR mod architecture runs as two cooperating processes:
1. The 32-bit in-process OpenGL hook (`opengl32.dll`) embedded inside `DarkAthena.exe`.
2. The 64-bit out-of-process OpenXR companion host (`riddickvr-host.exe`).
State is shared across processes via a high-performance memory-mapped IPC bridge (`RiddickVRSharedState`).
When `CameraHook::Update` toggled or calculated `cinemaMode`, `IpcBridgeClient::WriteState` was updating client tracking fields but omitted explicitly assigning `m_pSharedState->cinemaMode = state.cinemaMode`. As a result, the companion host compositor experienced frame latencies and missed menu transition signals.

## Decision
In `IpcBridgeClient::WriteState`, explicitly synchronize `m_pSharedState->cinemaMode = state.cinemaMode` on every swapchain write cycle.

## Consequences
- OpenXR host compositor transitions immediately between `XrCompositionLayerProjection` and `XrCompositionLayerQuad` with 0 dropped frames or visual stalls.
- State transitions are atomic and robust across process boundaries.
