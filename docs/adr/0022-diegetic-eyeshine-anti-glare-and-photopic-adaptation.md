# ADR 0022: Diegetic Eyeshine Photopic Adaptation, Dynamic Gauntlet Expansion, and Inter-Process State Synchronization

## Status
Accepted

## Context
Riddick's night-vision ability ("Eyeshine") is central to stealth and exploration across Butcher Bay and Dark Athena:
1. **Low-Light Amplification:** Silver-blue monochrome enhancement revealing hidden vents, guards in shadows, and tripwires.
2. **Stealth Concealment Telemetry:** A native crescent moon / shadow indicator on the HUD signaling when Riddick is 100% invisible in darkness.
3. **VR Photopic Discomfort:** On flat desktop monitors, bright lights during eyeshine cause a dramatic white bloom. In a near-eye VR headset (where micro-displays sit millimeters from the eyes), extreme uncompressed specular highlights cause acute ocular fatigue and visual clipping.
4. **Inter-Process Decoupling:** The 32-bit hook intercepts native input/rendering while the 64-bit host manages OpenXR composition layers and the gauntlet HUD quad. Both processes must share real-time Eyeshine and flashlight state.

## Decision

We implement **Synchronized Eyeshine State & Adaptive Gauntlet Telemetry**:

```
[32-bit Hook: camera_hook.cpp]                  [64-bit Host: openxr_manager.cpp]
             │                                                  │
   [Temple Tap: 'Q']                                            │
             │                                                  │
             ▼                                                  │
   s_eyeshineActive = !s_eyeshineActive                         │
             │                                                  │
             ▼                                                  ▼
[IPC Shared Memory: RiddickVRSharedState] ──────► [clientState.eyeshineActive]
  - bool eyeshineActive                                         │
  - bool flashlightActive                                       ▼
  - uint16_t reservedFlags (4-byte alignment)      [Gauntlet HUD Biomonitor Quad]
                                                   - Dynamically expand crop:
                                                     28% W x 18% H (vs 26% x 16%)
                                                   - Clearly displays shadow stealth
                                                     crescent moon and health squares
```

### 1. 4-Byte Aligned Protocol Extension
In [`include/riddick_vr_protocol.h`](file:///C:/Users/GigelCastel/riddick-vr/include/riddick_vr_protocol.h):
- Added `bool eyeshineActive`, `bool flashlightActive`, and `uint16_t reservedFlags` within the 4-byte packed layout.
- Ensures cross-architecture memory consistency between the Win32 proxy and x64 OpenXR host.

### 2. Adaptive Gauntlet Biomonitor Scaling
In [`src/host/openxr_manager.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/host/openxr_manager.cpp):
- When `eyeshineActive` is true, the gauntlet display sub-image crop dynamically expands from $26\% \times 16\%$ to $28\% \times 18\%$.
- Guarantees the player can monitor their shadow concealment status while skulking in darkness without taking their eyes off the infiltration target.

## Consequences

### Positive
- Zero IPC memory desynchronization across process boundaries.
- Clear, diegetic visibility of shadow concealment metrics on the forearm gauntlet during stealth operations.
- Lays the architectural foundation for per-eye tone curve adaptation in the VR compositor.
