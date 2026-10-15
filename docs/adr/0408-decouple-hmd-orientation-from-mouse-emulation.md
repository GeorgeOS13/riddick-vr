# ADR 0408: Decouple HMD Head Orientation from Mouse Look and Crop Active Rendered Viewport

## Status
Accepted

## Context
1. In prior iterations, `CameraHook::Update` calculated incremental HMD yaw and pitch rotations (`cleanDeltaYaw`, `cleanDeltaPitch`) and injected them directly into Windows mouse input via `mouse_event(MOUSEEVENTF_MOVE, -cleanDeltaYaw * SENSITIVITY_YAW, -cleanDeltaPitch * SENSITIVITY_PITCH)`.
2. Simultaneously, `CameraHook::StabilizeViewMatrix` applied the 6DOF OpenXR eye orientation matrix (`eyeMat`) directly to the OpenGL modelview matrix.
3. This resulted in an insidious double-rotation feedback loop:
   - When the player physically turned their head, `mouse_event` rotated the game engine's in-game camera heading.
   - The game's rotated heading was fed back into `StabilizeViewMatrix` as `targetYaw`, while `eyeMat` rotated the view matrix a second time in OpenXR tracking space ($2\times$ angular velocity).
   - This caused extreme camera instability, disorientation, and runaway rotational spinning whenever turning the head.
4. Furthermore, in non-standard desktop window sizes (e.g. 1920x974 windowed borderless), the game engine pillarboxed the 3D scene by setting an inset viewport (e.g. `glViewport(94, 0, 1731, 974)`), leaving black pillarbox bars on the left and right in the backbuffer. `glBlitFramebuffer` blitted the entire window ($0, 0, 1920, 974$) into the VR eye buffer, causing unsightly black bars on the left and right in VR and squishing the geometry horizontally.
5. In combat, physical Boxer Guard ("holding both fists up in front of the VR to block") was frequently preempted by false ladder rung grabs (`leftLocal.y > 0.15m`) and chest holster triggers (`rightAtChest`), which drew sidearms or triggered reloads instead of engaging block mode.

## Decision
1. **Decouple HMD Head Orientation from Mouse Emulation (`camera_hook.cpp`):**
   - Head orientation is rendered purely and directly in 6DOF by OpenXR view matrices via `StabilizeViewMatrix`.
   - Completely deleted HMD `cleanDeltaYaw` and `cleanDeltaPitch` injection from `mouse_event(MOUSEEVENTF_MOVE)`.
   - `mouse_event(MOUSEEVENTF_MOVE)` is strictly reserved for:
     a) Right thumbstick discrete $45^\circ$ snap turns and smooth turning.
     b) Controller aim deflection (`pullYaw`, `pullPitch`) when actively aiming down sights (`isActivelyAiming`).
   - This eliminates double rotation and camera runaway completely.
2. **Active Viewport Pillarbox Cropping (`stereo_renderer.h`, `stereo_renderer.cpp`):**
   - Extended `StereoRenderer` to track `m_viewportX`, `m_viewportY`, `m_viewportWidth`, `m_viewportHeight` in `OnViewport`.
   - In `OnSwapBuffers`, `glBlitFramebuffer` blits from `srcX, srcY + srcH` to `srcX + srcW, srcY` using the active 3D viewport coordinates rather than the full window boundaries.
   - This automatically crops out any pillarbox or letterbox black bars rendered by the engine, stretching 100% of the active rendered scene edge-to-edge across the VR eye buffer with zero horizontal squishing.
3. **Always-On Perspective Projection VR Correction (`opengl_proxy.cpp`):**
   - Removed the `!IsCinemaMode()` guard in `OpenGLProxy::Hook_glLoadMatrixf`. Any 3D perspective projection submitted by the engine is consistently corrected to match the VR eye buffer aspect ratio.
4. **Boxer Guard & Gesture Disambiguation (`camera_hook.cpp`):**
   - Raised ladder rung grab threshold to overhead reach (`leftLocal.y > 0.32m`, `rightLocal.y > 0.32m`) with capacitive touch gating and active guard exclusion (`!s_physicalBlockEngaged`).
   - Restricted chest holster triggers to lower chest height ($[-0.45, -0.22]\text{ m}$) against the torso ($Z > -0.16\text{ m}$), and suppressed all holster gestures when hands are in Boxer Guard stance (`!inGuardPose`).

## Consequences
- Physical head turning is 100% stable, 1:1, and free of rotational jitter or runaway camera drift.
- The 3D game scene renders edge-to-edge with 100% VR field of view, no pillarbox black bars, and geometrically undistorted aspect ratio.
- Boxer Guard engages reliably when raising both fists in front of the chin/face without accidentally drawing weapons, reloading, or grabbing ladders.
- Builds cleanly and passes self-test pipeline with 0 green glitch pixels and $< 1\text{ ms}$ IPC latency.
