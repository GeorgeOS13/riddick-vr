# ADR 0399: Binary Cinema Mode Switching & Tangent Space Aspect Matching

## Context
1. **Dynamic POV Switch Animation:**
   Previously, transitions between cinema mode (menus/cutscenes) and full VR projection (action gameplay) utilized a 150ms smooth transition alpha (`s_cinemaTransitionAlpha`). During this transition, the quad screen distance interpolated from 1.80m down to 0.85m while expanding in width from 2.0m to 3.6m and rendering both the projection and quad layers simultaneously. This created an unnatural expanding portal / zooming camera sensation ("POV switch") whenever cinema mode engaged or disengaged.
2. **Menu Stick Transition Conflict:**
   Left thumbstick deflection was previously configured to auto-transition out of cinema mode. In the main menu, when players used the thumbstick to navigate menu entries, this immediately disengaged cinema mode, causing the virtual cinema quad screen to disappear and breaking menu navigation.
3. **In-Game Squished Aspect Ratio & Black Bars:**
   Mapping the game's rendered 16:9 backbuffer into a nearly square per-eye buffer (~1:1 aspect) and projecting via raw lens FOV (`m_locatedViews[eye].fov`) compressed the horizontal axis by ~1.91x, squishing character models and world geometry.

## Decision
1. **Eliminate Dynamic POV Switch Interpolation:**
   - Completely remove `s_cinemaTransitionAlpha`, screen depth interpolation (`cinemaDist`), and dual-layer overlay rendering.
   - Enforce strict binary layer selection:
     - When `clientState.cinemaMode == true`: Render strictly `XrCompositionLayerQuad` at fixed distance 1.80m, fixed width 2.00m, and 16:9 aspect ratio.
     - When `clientState.cinemaMode == false`: Render strictly `XrCompositionLayerProjection` edge-to-edge.
2. **Isolate Menu Cinema Mode to Boot and Menus:**
   - In `CameraHook::Update`, if `!OpenGLProxy::Has3DGameplayProjection()` (boot, splash screens, title, and main menus before 3D gameplay has loaded), force `vrState.cinemaMode = true`. Thumbstick menu navigation never interrupts cinema mode.
   - Upon the first intercepted 3D gameplay projection matrix, automatically transition into full VR immersion (`cinemaMode = false`).
   - In-game pause menus are toggled via the Left Menu button or Button Y.
3. **Restore Tangent Space 16:9 Aspect Matching:**
   - In `OpenXRManager::RenderFrame`, calculate `projViews[eye].fov` tangent spans to enforce $\frac{S_x}{S_y} = \frac{16}{9}$ while strictly encompassing the physical headset lens aperture in all directions.
   - Completely eliminates horizontal geometric compression (circles and characters remain 100% round and undistorted) and guarantees zero black bars in gameplay.

## Consequences
- Menus and cutscenes render cleanly on a floating 16:9 virtual cinema screen at 1.8m.
- Gameplay renders edge-to-edge with 100% lens coverage, natural 1:1 scale, and zero black bars.
- Zero disorienting camera zooms or expanding portal transitions.
