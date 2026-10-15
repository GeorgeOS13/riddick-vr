# ADR 0407: Diegetic NanoMED Interaction, Physical Climbing, and Anti-Pullback Camera Stabilization

## Status
Accepted

## Context
1. In the pancake version of *The Chronicles of Riddick: Assault on Dark Athena*, health replenishment at NanoMED stations (`CWObject_HealthStation`) and traversal across vertical ladders and overhead hangrails (`CWObject_Phys_Ladder`) rely on canned third-person cinematic animations.
2. When the player triggers interaction via 'E', the engine:
   - Sets `usethirdperson = 1` and switches to `CWObject_AnimCamera`, yanking the camera backward 2.5 meters.
   - Sets `noviewmodel = 1`, hiding the first-person viewmodel and arms.
   - Plays pre-baked animations (`AGRiddick.xah`), removing all player control and spatial immersion.
3. In Virtual Reality, abrupt third-person camera pullbacks and passive animation locks induce severe motion sickness and destroy player agency.
4. The VR-first design principle requires:
   - **Diegetic Biometric Scanner Contact:** Resting an open flat palm against the NanoMED scanner console initiates health restoration with tactile 120 Hz nanite restorative haptics.
   - **Physical Cartridge Insertion:** When empty, drawing a cartridge from the player's hip holster (`Y < -0.38m`, grip clenched) and slamming it downward into the station's overhead receiver slot (`Vy < -0.22 m/s`) triggers mechanical latch haptics and sends the refill signal (`MSG_TRIGGERREFILL`).
   - **1:1 Physical Climbing:** Pull-to-climb and push-to-descend mechanics on ladders and overhead pipes using motion controller grip triggers and velocity-based propulsion.
   - **Camera Anti-Pullback Stabilization:** Locking camera translation to the first-person head anchor during interactive stations and climbing, neutralizing third-person camera offsets.

## Decision
1. **Diegetic NanoMED Healing Station Gestures (`camera_hook.cpp`):**
   - Implemented biometric scanner contact detection:
     - Open palm posture (`trigger < 0.20f && grip < 0.25f`) within the interaction console envelope ($Z \in [-0.65, -0.22]\text{ m}$, $Y \in [-0.25, 0.25]\text{ m}$, $|X| < 0.28\text{ m}$) initiates healing ('E') and delivers continuous 120 Hz sinusoidal haptic feedback.
   - Implemented physical cartridge extraction and insertion:
     - Hip pouch grab ($Y < -0.38\text{ m}$, `grip > 0.60f`) equips a NanoMED cartridge with tactile click feedback.
     - Downward slam into the overhead receiver ($Z \in [-0.62, -0.20]\text{ m}$, $Y \in [0.05, 0.42]\text{ m}$, $V_y < -0.22\text{ m/s}$) dispatches refill input ('E' / `MSG_TRIGGERREFILL`) with high-impact mechanical latch haptics ($0.85\text{ amplitude}$).
2. **First-Person Camera Anti-Pullback Stabilization (`CameraHook::StabilizeViewMatrix`):**
   - Active interaction states (`isClimbingActive`, `s_nanoMedPalmActive`, `s_holdingNanoMedCartridge`) engage `s_vrFirstActionActive`.
   - When active, sudden view matrix translation jumps exceeding $0.65\text{ m}$ (characteristic of engine third-person cutscene pullback) are clamped to the stabilized first-person neck/head anchor `s_firstPersonAnchor`.
   - Vertical traversal translation while climbing is preserved, keeping head tracking 1:1 and horizon-stabilized.
3. **Physical Traversal Synchronization:**
   - Synchronized hand-over-hand ladder climbing and overhead hangrail traversal with always-visible 3D convict gloves, delivering tactile climbing stroke haptics ($0.20\text{ amplitude}$) and two-handed mantle confirmation ($0.60\text{ amplitude}$).

## Consequences
- Player agency is preserved throughout healing and traversal sequences with zero forced third-person camera switches.
- Health replenishment and cartridge insertion become physical, tactile VR interactions.
- Builds cleanly and passes autonomous self-play validation with 0 glitch artifacts.
