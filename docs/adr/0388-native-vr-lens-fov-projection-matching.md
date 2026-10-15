# ADR 0388: Native VR Lens FOV ProjectionFrustum Matching

## Context
In `src/host/openxr_manager.cpp`, the action mode projection views (`projViews[eye].fov`) were being artificially constrained to a 16:9 aspect ratio (`TARGET_ASPECT = 16.0f / 9.0f`) instead of matching the physical lens FOV (`m_locatedViews[eye].fov`).
Because modern VR headset displays (such as Meta Quest 2 / Quest 3) have an approximately square per-eye aspect ratio (~1:1), forcing a 16:9 frustum into `XrCompositionLayerProjection` caused the OpenXR runtime to render black letterbox bars on the top and bottom of the headset lenses in gameplay.

## Decision
1. In action mode (`!clientState.cinemaMode`), map the projection layer FOV directly to the headset's native physical lens FOV:
   `projViews[eye].fov = m_locatedViews[eye].fov;`
2. Configure the swapchain subImage to encompass the entire allocated eye buffer:
   `imageRect.offset = {0, 0};`
   `imageRect.extent = { width, height };`
3. In `stereo_renderer.cpp`, blit the backbuffer edge-to-edge `(0, 0, m_eyeWidth, m_eyeHeight)` in action mode.

## Consequences
- Completely eliminates all black bars inside the VR headset during gameplay.
- Delivers 100% immersive, edge-to-edge VR coverage across the physical lenses.
