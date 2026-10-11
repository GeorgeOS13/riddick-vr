# ADR 0002: Dual-Process IPC & 64-bit OpenXR Bridge

## Status
Accepted

## Context
`DarkAthena.exe` is a 32-bit (x86) executable. In contrast, modern VR hardware and PC VR runtimes (Meta Quest Link, SteamVR, Virtual Desktop, Pimax, WMR) are overwhelmingly built around 64-bit (x64) binaries and loaders.

Historically, retro VR mods attempting to load 32-bit OpenXR loaders in-process face several severe issues:
1. Vendor 32-bit OpenXR loaders are frequently neglected, buggy, or outright absent in modern runtime distributions.
2. In-process crashes inside proprietary VR runtime layers take down the entire game process.
3. 32-bit memory space is constrained to 2GB–4GB (with LAA / Large Address Aware), creating risk of Out-Of-Memory (OOM) errors when allocating high-resolution stereoscopic VR render targets.

*F.E.A.R. VR* (DR-89) effectively mitigated this by implementing an x64 OpenXR companion host process communicating with the 32-bit game hook via inter-process communication (IPC).

## Decision
We adopt a **Dual-Process Architecture**:
1. **Client / Injector (32-bit x86 `opengl32.dll` / hook DLL):**
   - Injected into `DarkAthena.exe`.
   - Intercepts OpenGL draws, camera updates, and game inputs.
   - Allocates dual eye FBOs (or renders into shared textures).
   - Communicates with the Host process via high-speed shared memory.
2. **Companion Host (64-bit x64 `RiddickVRHost.exe`):**
   - Native 64-bit binary running outside the game address space.
   - Interfaces directly with the system's active 64-bit OpenXR runtime (`openxr_loader.dll`).
   - Handles HMD session lifecycle, space tracking, frame timing (`xrWaitFrame`, `xrBeginFrame`, `xrEndFrame`), and motion controller input polling.
   - Receives rendered eye textures via shared GPU texture handles (DXGI shared handle / D3D11 / Vulkan interop).
3. **IPC Transport:**
   - Double-buffered shared memory mapped file (`CreateFileMappingA` / `MapViewOfFile`) for frame metadata, pose tracking matrices, and input state.
   - Named mutexes and events for low-latency synchronization (sub-millisecond lock times).

```
┌─────────────────────────────────┐           ┌─────────────────────────────────┐
│     DarkAthena.exe (32-bit)     │           │    RiddickVRHost.exe (64-bit)   │
│                                 │           │                                 │
│  ┌───────────────────────────┐  │           │  ┌───────────────────────────┐  │
│  │ opengl32.dll Proxy Hook   │  │           │  │ OpenXR 64-bit Session     │  │
│  │ - Intercepts Matrices     │  │           │  │ - Headset 6DOF Tracking   │  │
│  │ - Renders Stereo Eyes     │  │           │  │ - Motion Controller Input │  │
│  │ - Writes Shared Textures  │  │           │  │ - Frame Scheduling        │  │
│  └─────────────┬─────────────┘  │           │  └─────────────┬─────────────┘  │
└────────────────┼────────────────┘           └────────────────┼────────────────┘
                 │                                             │
                 │◄──────────── Shared Memory IPC ─────────────┤
                 │   - Head Pose / Eye Matrices (Host -> Hook) │
                 │   - Controller Input State (Host -> Hook)   │
                 │   - Frame Sync Signals (Events/Semaphores)  │
                 │                                             │
                 │◄───────── Shared Texture Handle ────────────┤
                 │   - Left & Right Eye GPU Render Targets     │
```

## Consequences
### Positive
- Zero compatibility issues with 64-bit OpenXR runtimes and headsets.
- Memory isolation: VR swapchains and compositor textures do not exhaust the 32-bit game's virtual address space.
- The VR host can gracefully recover from headset sleep/wake cycles or runtime restarts without crashing the game engine.

### Negative / Challenges
- Requires managing two processes and implementing robust process synchronization.
- Initial frame synchronization must be carefully tuned to prevent input-to-photon latency spikes.
