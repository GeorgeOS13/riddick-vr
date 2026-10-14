# ADR 0010: Diegetic Weapon Ammo Receiver HUD and Adaptive Stealth Wall-Peeking

## Status
Accepted

## Context
*The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena* feature iconic atmospheric stealth and close-quarters gunplay. Two systems are crucial for VR presence:

1. **Weapon Status and Ammo Visibility:**
   In flat-screen gameplay, ammo count and magazine capacity are statically overlaid in the bottom-right HUD corner. In VR action mode, requiring the player to look down into the lens periphery causes chromatic aberration, lens edge distortion, and breaks gunplay flow. Immersive VR shooters (such as *Dead Space VR*, *Half-Life: Alyx*, and *Doom 3 BFG VR*) integrate the ammo readout diegetically onto the firearm itself.

2. **Stealth Wall-Peeking and Torso Decoupling:**
   Corner peeking and shadow concealment are foundational to Riddick's predatory identity. Players naturally expect to lean laterally around doorways, crates, and corners to spot patrolling guards.
   However:
   - If head translation directly drags the player's collision capsule, leaning exposes Riddick's entire body to enemy line of sight.
   - Conversely, if head translation is completely detached from the collision capsule, the player could walk through walls in roomscale without collision response.

## Decision

We implement a **Dual-Module Diegetic Display Architecture** and an **Adaptive Kinematic Lean Engine**:

```
                 [OpenXR Tracking & Viewports]
                               │
         ┌─────────────────────┴─────────────────────┐
         ▼                                           ▼
[Diegetic Weapon Ammo Receiver]             [Adaptive Stealth Wall-Peeking]
- Anchored to right controller aim pose     - Calibrated Torso Base (X, Z)
- Offset: (0.0, +0.025m, +0.060m)           - Lateral Lean: |Lean_right| > 0.15m
- Sight alignment cone (> 0.35 dot)         - 0.15 Tactile stealth haptic cue
- Cropped from swapchain bottom-right       - [0, 0.38m] Floor capsule deadband
- 5.5cm x 4.0cm weapon backplate display    - Dynamic torso drag beyond 0.38m
```

### 1. Diegetic Weapon Ammo Counter & Holo-Sight HUD (`src/host/openxr_manager.cpp`)
- **Spatial Attachment:**
  An auxiliary `XrCompositionLayerQuad` (`weaponAmmoQuadLayer`) is parented to `clientState.rightController.aimPose` (or `gripPose` fallback):
  - Local offset: $X = 0.0\text{ m}, Y = +0.025\text{ m}, Z = +0.060\text{ m}$ (placing the quad directly on the rear slide backplate / receiver housing).
  - Normal orientation: Rotated $180^\circ$ around the controller's $Y$ axis ($Q_{rel} = (0, 1, 0, 0)$), pointing the quad normal rearward directly into the shooter's eyes.
- **Sight Alignment Cone:**
  Evaluates the normalized vector from the weapon backplate to the player's HMD:
  $$\text{ammoGlanceDot} = \vec{N}_{quad} \cdot \left(\frac{\vec{P}_{hmd} - \vec{P}_{weapon}}{\|\vec{P}_{hmd} - \vec{P}_{weapon}\|}\right)$$
  When $\text{ammoGlanceDot} > 0.35$ (within a $\approx 70^\circ$ viewing cone), the display is activated.
- **Zero-Copy Swapchain Sub-Image:**
  Crops the bottom-right HUD region ($23\% \times 23\%$) from the primary eye swapchain, mapping it onto a $5.5\text{ cm} \times 4.0\text{ cm}$ high-contrast weapon backplate display without additional GPU render passes.

### 2. Adaptive Stealth Wall-Peeking (`src/hook/camera_hook.cpp`)
- **Torso Base Calibration:**
  A reference torso anchor $(X_{torso}, Z_{torso})$ is initialized on level transition or recentering.
- **Coordinate Projection in Stabilized Heading:**
  Horizontal head displacement $\vec{D} = (X_{hmd} - X_{torso}, Z_{hmd} - Z_{torso})$ is projected into the player's stabilized yaw frame:
  $$\text{Lean}_{right} = D_x \cos(\theta_{yaw}) - D_z \sin(\theta_{yaw})$$
  $$\text{Lean}_{fwd} = -D_x \sin(\theta_{yaw}) - D_z \cos(\theta_{yaw})$$
- **Corner Peek Detection:**
  When $|\text{Lean}_{right}| > 0.15\text{ m}$, the system registers **Stealth Wall-Peek Stance**, pulsing subtle tactile feedback ($0.15$ amplitude) to confirm the player is successfully leaning around cover.
- **Collision Boundary Retention (Deadband):**
  Within a radius of $[0, 0.38\text{ m}]$, Riddick's floor collision capsule remains safely anchored behind wall cover. When physical displacement exceeds $0.38\text{ m}$, the torso anchor smoothly slides along the displacement vector, keeping the body synchronized and preventing roomscale detachment or wall clipping.

## Consequences

### Positive
- **Cohesive Dual Diegetic HUD:** Left gauntlet biomonitor displays 4-box health; right weapon receiver displays magazine ammo and firing mode.
- **Intuitive Stealth Mechanics:** True physical leaning around Butcher Bay corridor corners and catwalk pillars without exposing character hitboxes.
- **Vestibular Comfort:** Retains locked world-vertical horizon and bounded body presence without rotational or positional jarring.
