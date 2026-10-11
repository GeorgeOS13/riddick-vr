# ADR 0003: Stereoscopic Double-Pass Rendering & Stencil Shadow Volumes

## Status
Accepted

## Context
A standard flat monitor displays a single perspective projection per frame. For true stereoscopic 3D Virtual Reality, two independent perspectives must be rendered each frame:
1. **Left Eye View:** Camera shifted by $-IPD / 2$ along the horizontal inter-pupillary baseline, utilizing the Left Eye asymmetric frustum (`XrFovf`).
2. **Right Eye View:** Camera shifted by $+IPD / 2$ along the horizontal inter-pupillary baseline, utilizing the Right Eye asymmetric frustum (`XrFovf`).

The Starbreeze Engine (`sbengine`) utilizes an advanced lighting model built around **stencil shadow volumes**, normal-mapped surfaces, and screen-space passes. In flat rendering, shadow volume silhouettes are extruded from occluding geometry and rendered into a shared stencil buffer to mark shadow coverage.

If stencil shadows or screen-space passes are rendered using a flat camera or shared buffers across both eyes, severe visual artifacts occur: shadows will appear at wrong depths, detach from physical objects, or render differently in each eye (retinal rivalry).

## Decision
1. **Double-Pass World Injection:**
   - Rather than relying on simple post-process reprojection or depth-based stereo synthesis (which causes haloing and disocclusion artifacts), we will execute true **geometry-level double-pass rendering**.
   - When the main scene render loop begins, our hook will intercept the camera setup routine and execute two passes:
     - **Pass 1 (Left Eye):** Bind Left Eye FBO $\to$ inject Left View & Asymmetric Projection Matrix $\to$ render world geometry.
     - **Pass 2 (Right Eye):** Bind Right Eye FBO $\to$ inject Right View & Asymmetric Projection Matrix $\to$ render world geometry.
2. **Isolated Stencil & Depth Buffers:**
   - Each eye will maintain its own discrete depth-stencil buffer attachment.
   - Stencil shadow volume extrusion and clipping calculations will be performed with respect to the per-eye camera position, ensuring shadow edges align with true stereoscopic geometry.
3. **Optimizing Redundant Work:**
   - Pure world-space passes that do not depend on the viewer's eye position (e.g. static environment shadow maps, world cubemap reflections, light grid evaluations) will only be computed once per frame and shared across both eye passes.

## Consequences
### Positive
- Crisp, geometrically accurate stereoscopic depth with zero disocclusion artifacts.
- Stencil shadows accurately project onto surfaces matching natural ocular parallax.

### Negative / Challenges
- Double-pass world rendering incurs approximately $1.6\times$ to $1.9\times$ draw call load on the CPU and GPU.
- Requires identifying the boundary within the engine between world-space calculations and screen/view-space draw calls.
