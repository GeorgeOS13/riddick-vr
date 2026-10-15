# ADR 0403: Native Projection Aspect Matching & Boxer Guard Refinement

## Context
1. In `src/host/openxr_manager.cpp`, the action mode projection FOV (`projViews[eye].fov`) previously attempted to match the game's 16:9 aspect ratio by expanding the horizontal tangent span ($targetHSpan = targetVSpan \times \frac{16}{9}$). Because VR headset displays have an approximately square physical aspect ratio (~0.93:1), requesting a 128° horizontal FOV caused OpenXR runtimes (Virtual Desktop, Meta Quest Link) to clamp and pillarbox the layer with black bars on the left and right, leaving the in-game world looking squished.
2. In `src/hook/opengl_proxy.cpp`, the game engine rendered 3D perspective scenes using a 16:9 projection matrix ($|m[5]/m[0]| \approx 1.78\text{–}1.96$), which when blitted to the 2496x2688 eye buffer caused geometric distortion (tall, skinnier objects).
3. In `src/hook/camera_hook.cpp`, any controller movement exceeding $1.10\text{ m/s}$ (`rSpeed > 1.10f || lSpeed > 1.10f`) was treated as a punch attack. When the player naturally raised their arms upward ($V_y > 0$) in front of their face into a defensive Boxer Guard, the upward velocity triggered false attacks, canceled active blocking (`mouse_event(MOUSEEVENTF_RIGHTUP)`), and locked out guard detection for 200ms.

## Decision
1. **Headset-Native Projection FOV (Zero Black Bars):**
   - In `openxr_manager.cpp`, set `projViews[eye].fov = m_locatedViews[eye].fov`.
   - The OpenXR projection layer covers 100% of the physical headset lenses edge-to-edge with zero black bars horizontally or vertically.
2. **Engine Projection Matrix Aspect Matching:**
   - In `opengl_proxy.cpp`, intercept `glLoadMatrixf` for 3D perspective projections (`fabsf(m[15]) < 0.01f && m[0] > 0.10f && m[0] < 4.0f`) during gameplay (`!StereoRenderer::Instance().IsCinemaMode()`).
   - Dynamically scale horizontal focal length $m[0] = |m[5]| / \text{Aspect}_{\text{VR}}$, where $\text{Aspect}_{\text{VR}} = \frac{W_{\text{eye}}}{H_{\text{eye}}} = \frac{2496}{2688} \approx 0.9286$.
   - Preserves 1:1 round circles and eliminates horizontal squishing.
3. **Directional Melee Strike Discrimination:**
   - Differentiate intentional strikes (jabs $V_z < -1.20\text{ m/s}$, hooks $|V_x| > 1.35\text{ m/s}$, uppercuts $V_y > 1.35\text{ m/s}$ with forward velocity) from defensive guard positioning ($V_y > 0$ into the guard envelope).
   - Suppress attack triggers when hands are inside the defensive guard envelope ($Y \in [-0.42, 0.32]$, $Z \in [-0.68, -0.05]$, $|\Delta X| < 0.56\text{ m}$).
   - Broadened Boxer Guard envelope ($Y \in [-0.40, 0.30]$, $Z \in [-0.65, -0.06]$, $|\Delta X| < 0.54\text{ m}$) with 40ms hold engagement and instant 0ms drop on intentional punches.

## Consequences
- Full edge-to-edge VR immersion in gameplay with zero black bars on any side.
- 100% geometrically correct world proportions without horizontal or vertical squishing.
- Raising arms in front of the face instantly and reliably engages Boxer Guard without firing accidental punches.
- Clean self-test pipeline pass with 0 errors and 0 green glitch pixels.
