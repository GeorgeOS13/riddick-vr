# ADR 0405: Unified Single-POV Projection Layer and Eyeshine Remapping

## Context
1. In `src/host/openxr_manager.cpp`, the compositor previously switched between `XrCompositionLayerQuad` (virtual cinema screen) when `clientState.cinemaMode` was true and `XrCompositionLayerProjection` (immersive 3D VR) when false.
2. In full-screen gameplay, `GetCursorInfo` frequently toggled `CURSOR_SHOWING` on and off during UI rendering, causing `vrState.cinemaMode` to alternate rapidly. This triggered violent frame-by-frame POV flashing between quad and projection layers.
3. Left controller Button Y previously had a manual toggle for `s_cinemaModeState`. Pressing Y during gameplay engaged cinema mode, projecting letterboxed black bars across the display and squishing the rendered image.
4. The user explicitly requested:
   - Eliminate POV switching entirely.
   - Retain black bars strictly in menus (rendered into the swapchain) without switching layer types.
   - Remove cinema mode toggling from Button Y, and eliminate black bars in gameplay.

## Decision
1. **Unified Single-POV Projection Architecture:**
   - Completely removed `XrCompositionLayerQuad` and the dual-layer composition branch in `openxr_manager.cpp`.
   - OpenXR now exclusively renders `XrCompositionLayerProjection` at all times across both menus and gameplay.
   - In menus and boot screens, `stereo_renderer.cpp` centers the 16:9 framebuffer with black pillarbox borders directly inside the projection swapchain.
   - In gameplay, `stereo_renderer.cpp` blits 1:1 edge-to-edge into the full eye buffer, providing complete VR immersion without black bars.
2. **Deterministic Mode Discipline:**
   - In `camera_hook.cpp`, removed `GetCursorInfo` polling and cursor-based flickering.
   - Mode state is strictly tied to 3D matrix initialization: `vrState.cinemaMode = !has3D;`. Once the 3D perspective matrix is engaged by the game engine, `cinemaMode` remains permanently false.
3. **Button Y and Menu Button Remapping:**
   - Removed cinema mode toggling from Button Y. Button Y is now dedicated to Riddick's Eyeshine Night Vision ('Q') accompanied by a tactile haptic pulse (`vrState.leftHapticPulse = 0.35f`).
   - Removed cinema mode toggling from the Left Controller Menu Button. The menu button now cleanly taps `VK_ESCAPE` to pause the game.

## Consequences
- Eliminates compositor POV switching and frame flashing.
- Retains clean 16:9 black-barred menus on boot and pause screens.
- Delivers uncompromised edge-to-edge VR immersion in gameplay with zero black bars.
- Restores tactile access to Riddick's signature Eyeshine mechanic via Button Y.
