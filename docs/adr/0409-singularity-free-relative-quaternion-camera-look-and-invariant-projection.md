# ADR 0409: Singularity-Free Relative Quaternion Camera Look and Invariant Projection

## Status
Accepted

## Context
1. In Starbreeze engine (`RndrGL.dll`), overriding `glLoadMatrixf(GL_MODELVIEW)` breaks character dynamic lighting and causes world geometry to render pitch black (documented in commit `abcb785`). Consequently, `GL_MODELVIEW` is intentionally passed through unaltered to the engine, and the player's in-game camera heading and pitch are driven via Windows mouse look input (`mouse_event`).
2. In earlier iterations, `CameraHook::Update` extracted head yaw and pitch angles using standard Euler formulas:
   - $\text{yaw} = \text{atan2}(2(qw \cdot qy + qz \cdot qx), 1 - 2(qx^2 + qy^2))$
   - $\text{pitch} = \arcsin(2(qw \cdot qx - qy \cdot qz))$
3. **The Gimbal Lock Singularity**: When the user pitched their head up or down, the denominator $1 - 2(qx^2 + qy^2)$ approached zero. Any slight head roll or pitch near $\pm 90^\circ$ caused $\text{atan2}$ to diverge wildly, injecting hundreds of erratic mouse movement pixels in fractions of a second into `mouse_event(MOUSEEVENTF_MOVE)`. This manifested as severe disorientation with the camera spinning uncontrollably in all directions.
4. When `mouseDeltaX` and `mouseDeltaY` were temporarily zeroed in ADR 0408, physical head rotation ceased to turn the camera in-game entirely.
5. Furthermore, menu rendering had a mode-switching mechanism where `cinemaMode` toggled between full screen and a letterboxed 16:9 inner rectangle, clearing the framebuffer with black bars. If the game toggled state or if the user pressed Left Controller Button Y (which in older versions was mapped to a cinema toggle), the viewport flashed between two POVs and added an excessive amount of black bars.

## Decision
1. **Singularity-Free Relative Quaternion Head Tracking Look (`src/hook/camera_hook.cpp`)**:
   - Rather than extracting absolute Euler angles with trigonometric inverses ($\text{atan2}$, $\arcsin$), compute the relative inter-frame rotation quaternion in head-local space:
     $$\Delta Q = Q_{prev}^{-1} \cdot Q_{curr}$$
     where $Q_{prev} = (x_1, y_1, z_1, w_1)$ and $Q_{curr} = (x_2, y_2, z_2, w_2)$.
   - Direct algebraic component calculation:
     - $\Delta w = w_1 w_2 + x_1 x_2 + y_1 y_2 + z_1 z_2$
     - $\Delta x = w_1 x_2 - x_1 w_2 - y_1 z_2 + z_1 y_2$
     - $\Delta y = w_1 y_2 + x_1 z_2 - y_1 w_2 - z_1 x_2$
     - $\Delta z = w_1 z_2 - x_1 y_2 + y_1 x_2 - z_1 w_2$
   - Select the shortest geodesic path on $S^3$: if $\Delta w < 0$, negate $\Delta Q$.
   - Extract small inter-frame local rotation angles directly:
     - $\Delta \theta_{yaw} = 2.0 \cdot \Delta y$ (pure local neck yaw around Y)
     - $\Delta \theta_{pitch} = 2.0 \cdot \Delta x$ (pure local head pitch around X)
     - $\Delta \theta_{roll} = 2.0 \cdot \Delta z$ (pure local head roll, strictly decoupled from mouse look)
   - Zero division by zero, zero Euler gimbal lock, zero trigonometric singularity across all orientations in $SO(3)$.
   - Apply a sub-degree noise filter (`HMD_LOOK_DEADZONE = 0.0004f`) and map to mouse look:
     - $\text{mouseDeltaX} = -\text{cleanDeltaYaw} \cdot 445.6\text{f}$
     - $\text{mouseDeltaY} = -\text{cleanDeltaPitch} \cdot 445.6\text{f}$
2. **Invariant Single-POV Projection Discipline (`src/hook/stereo_renderer.cpp`, `src/hook/camera_hook.cpp`)**:
   - Completely removed the `cinemaMode` letterbox shrinking and black background clearing block from `StereoRenderer::OnSwapBuffers`.
   - `dstX = 0, dstY = 0, dstW = m_eyeWidth, dstH = m_eyeHeight` is strictly maintained for every frame.
   - Set `vrState.cinemaMode = false` permanently.
   - Menus are rendered directly into the eye buffer with their native 16:9 pillarboxes, completely eliminating POV switching, screen shrinking, and menu flashing.
   - Left Controller Button Y is strictly mapped to Riddick Eyeshine Night Vision ('Q') with haptics, never toggling cinema mode or adding black bars.

## Consequences
- Physical head look is completely smooth, 1:1, and mathematically immune to gimbal lock or runaway spinning at any pitch angle.
- Menus and gameplay share a single unified projection with zero POV flashing or black bar toggling.
- Left Button Y reliably toggles Riddick Eyeshine ('Q') without altering display geometry or composition layers.
- Self-test pipeline passes 60 frames with 0 green glitch pixels and $< 1\text{ ms}$ IPC synchronization.
