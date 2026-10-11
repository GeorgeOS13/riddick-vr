# ADR 0006: Diegetic HUD, Eyeshine & Post-Processing Shaders

## Status
Accepted

## Context
*The Chronicles of Riddick* utilized advanced visual post-processing effects and a groundbreakingly minimalist user interface:
1. **Eyeshine (Night Vision):** Riddick's signature ability toggles a full-screen shader pass that desaturates colors, adds a silvery monochrome tint, blooms bright light sources, and increases contrast on specular highlights.
2. **Post-Processing Suite:** Motion blur, heavy film grain, depth-of-field, and hit flash effects.
3. **HUD Design:** Unlike other shooters with cluttered screens, Riddick features almost no permanent HUD—only a four-segment health box in the top-left corner, a dynamic reticle that fades out when not aiming, and stealth indicator vignettes.

In VR, screen-space 2D elements and flat post-processing cause severe issues:
- Flat camera-space motion blur causes severe simulator sickness.
- 2D HUD drawn directly into the backbuffer appears glued to the user's eyeballs at zero focal depth, causing extreme eyestrain (vergence-accommodation conflict).
- Fullscreen shaders applied as a single 2D quad across the combined swapchain will distort depth perception.

## Decision
1. **Post-Processing Adjustments:**
   - **Camera Motion Blur:** Disabled via engine config or shader intercept. Rotational blur in VR must be handled by headset display persistence, not software blur.
   - **Film Grain & Depth of Field:** Disabled or scaled down to preserve image clarity on high-resolution VR panels.
   - **Eyeshine Shader Adaptation:** The Eyeshine shader pass will be intercepted and applied independently to both the Left Eye and Right Eye FBOs before compositor submission. The specular highlight boost will be preserved to maintain the game's distinctive aesthetic.
2. **HUD Projection (Floating Stereo Quad / Wrist Display):**
   - Intercept 2D orthogonal UI draw calls (`glOrtho` or GUI render passes).
   - Redirect 2D UI rendering into an off-screen render target.
   - Project this UI texture in world-space as a **floating, slightly curved quad** positioned:
     - Distance: $1.8\text{ m}$ ahead of the player camera.
     - Scale & Depth: Scaled to occupy a comfortable field-of-view without clipping through nearby walls (using depth testing with gentle offset).
   - **Optional Diegetic Mode:** Map the health block indicators to the back of Riddick’s left glove/wrist (accessible by glancing down at the hand).

## Consequences
### Positive
- Preserves the iconic atmospheric feel of Eyeshine in low-light prison stealth sections.
- Eliminates nausea caused by 2D camera motion blur.
- The minimalist HUD makes the transition to VR exceptionally natural.

### Negative / Challenges
- Fullscreen shader uniform variables must be kept in sync between the Left and Right eye passes to avoid lighting inconsistencies between eyes.
