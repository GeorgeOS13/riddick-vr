# ADR 0384: DirectSound & OpenAL 3D Spatial Audio Listener Yaw Synchronization

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*, environmental audio (inmate voices, ventilation hums, guard footsteps, and ricochets) is spatialized using Starbreeze's internal audio listener matrix in `MSystem.dll`. Because VR decouples HMD head rotation from the game engine's mouse turning, when the player looks around physically with their headset, sub-pixel rounding in mouse delta accumulation or discrete snap-turning steps could allow the engine's internal camera yaw to drift slightly from the player's true physical HMD forward orientation.

When listener yaw diverges from headset yaw, audio cues (e.g., a drone approaching from the left) can sound panned at the wrong angle relative to the visual stereo image in the HMD.

## Decision
Implement closed-loop continuous audio listener yaw synchronization in `src/hook/camera_hook.cpp`:

1. **Cumulative Listener Yaw Tracking**:
   - Track cumulative in-engine camera yaw:
     $$s_{\text{engineAudioListenerYaw}} \mathrel{+}= \frac{-\Delta_{\text{mouseLookX}}}{\text{SENSITIVITY\_YAW}}$$
   - Wrap both physical HMD yaw and engine listener yaw within $[-\pi, +\pi]$.

2. **Angular Divergence Gating**:
   - Compute angular delta:
     $$\Delta_{\text{yawError}} = \text{currentYaw} - s_{\text{engineAudioListenerYaw}}$$
   - If $|\Delta_{\text{yawError}}| > 0.015\text{ rad}$ ($\sim 0.85^\circ$), inject a proportional corrective micro-delta ($12\%$) into the mouse accumulation buffer:
     $$\text{syncCorrection} = \Delta_{\text{yawError}} \times 0.12 \times \text{SENSITIVITY\_YAW}$$
     $$\text{mouseDeltaX} \mathrel{-}= \text{syncCorrection}$$

## Consequences
- Guaranteed $< 1^\circ$ alignment between the physical HMD orientation and the engine's 3D spatial audio listener.
- Pinpoint acoustic localization for stealth gameplay, corridor acoustics, and enemy detection.
- Zero perceptual lag or audio panning misalignment during rapid head rotations.
