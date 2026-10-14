# ADR 0007: Full Viewport Immersion & FOV Projection Matching

## Status
Accepted

## Context
In *The Chronicles of Riddick* VR mod, rendering stereoscopic eye buffers from the game's OpenGL backbuffer presented a fundamental trade-off between aspect ratio preservation, HUD visibility, and VR immersion:

1. **The Letterbox Bar Problem (Commit `3ad979c`):**
   To keep the game's native 16:9 HUD visible within the central lens sweet spot of modern headsets (such as Meta Quest 2/3 with tall ~0.93 aspect 2496x2688 per-eye buffers), a 16:9 centered letterbox blit was introduced. In `XrCompositionLayerProjection`, this left solid black bars above and below (and/or to the sides) of the rendered world, severely degrading VR presence and immersion.

2. **The "Way Too Zoomed" Problem:**
   Blitting the game's 16:9 backbuffer directly across the entire tall eye buffer (`0, 0, eyeWidth, eyeHeight`) without projection compensation caused two distinct optical distortions:
   - **Severe Vertical Magnification:** Riddick's native vertical FOV (~45°–48°) was mapped across the headset's large vertical lens FOV (~104°), making objects appear over 2x larger than life and uncomfortably close to the player's eyes.
   - **Non-Uniform Aspect Squashing:** A 1.78 aspect image mapped onto a 0.93 aspect display stretched the vertical axis by ~1.91x relative to the horizontal axis, distorting spherical geometry into tall ovals.

3. **Virtual Cinema Quad Success:**
   The Floating Virtual Cinema Quad (`XrCompositionLayerQuad`) introduced for menus and cutscenes received high praise from players for readability and stability. This mode must be preserved for menus and cutscenes.

## Decision

1. **Dual-Mode Blitting in `StereoRenderer::OnSwapBuffers`:**
   - **Cinema Mode (`m_sharedState.cinemaMode == true`):** Preserve the 16:9 centered crop (`dstX, dstY, dstW, dstH`) and clear outer borders to solid opaque black. The OpenXR host samples this exact region onto the 16:9 virtual floating screen quad.
   - **Gameplay / Action Mode (`m_sharedState.cinemaMode == false`):** Blit the entire game viewport edge-to-edge into the full eye framebuffer (`dstX = 0, dstY = 0, dstW = m_eyeWidth, dstH = m_eyeHeight`). No black border clearing is performed, ensuring every pixel of the eye buffer contains active rendered geometry.

2. **Aspect-Matched Un-zoomed Projection in `OpenXRManager::RenderFrame`:**
   Instead of projecting onto the raw physical lens FOV (`m_locatedViews[eye].fov`), configure `projViews[eye].fov` in tangent space:
   - **Un-zoom Factor:** Scale vertical angular tangent span by `ACTION_UNZOOM_FACTOR = 1.15f` to bring in-game scale down to natural 1:1 real-world human proportions.
   - **Aspect Ratio Matching:** Enforce `targetHSpan = targetVSpan * (16.0f / 9.0f)`. This guarantees that the angular ratio between horizontal and vertical axes matches the game's perspective projection matrix, completely eliminating aspect stretching and keeping circles perfectly round.
   - **Full Lens Coverage:** Verify that `targetHSpan >= hSpanBase * ACTION_UNZOOM_FACTOR` and `targetVSpan >= vSpanBase * ACTION_UNZOOM_FACTOR`. Because the projection frustum strictly encompasses the physical lens aperture in all directions (`angleLeft <= baseLeft`, `angleRight >= baseRight`, `angleUp >= baseUp`, `angleDown <= baseDown`), the visible lens area is 100% filled by the virtual world with zero black bars.

## Consequences

### Positive
- **Complete In-Game Immersion:** Eliminates all black bars in VR action gameplay; the game world fully occupies the player's field of view.
- **Natural 1:1 Scale:** Eliminates the uncomfortable 2x zoom effect without introducing geometric aspect distortion.
- **Distortion-Free Optics:** Spherical models, reticles, and human proportions remain geometrically true across both axes.
- **Seamless Cinema Transitions:** Menus and cutscenes continue to utilize the floating cinema quad screen with 16:9 letterbox preservation.

### Negative / Trade-offs
- Rendering the full buffer across an expanded FOV means the extreme corners of the rendered frame extend slightly beyond the visible lens aperture. HUD elements positioned at the absolute screen extremities are viewed comfortably via natural head look.
