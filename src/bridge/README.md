# IPC Bridge Component

## Responsibility
The Bridge component defines the inter-process communication protocol and shared memory layouts between:
1. `DarkAthena.exe` (32-bit x86 client process)
2. `RiddickVRHost.exe` (64-bit x64 companion host process)

## Key Duties
- Manage the double-buffered shared memory file mapping (`CreateFileMappingA` / `MapViewOfFile`).
- Synchronize frame pacing using named Windows events and mutexes.
- Exchange data packets:
  - **Host $\to$ Hook:** Headset 6DOF pose, eye view/projection matrices, controller inputs, vibration requests.
  - **Hook $\to$ Host:** Frame readiness flags, shared GPU texture handles, performance telemetry.
