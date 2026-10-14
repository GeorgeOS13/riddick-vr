# ADR 0062: Cybernetic Drone HUD Decoupled Sight & Reticle Drift Dampening

## Status
Accepted / Implemented

## Context
When piloting remote Ghost Drones or manning mounted gun emplacements in *Assault on Dark Athena*, traditional flat-screen camera rotation binds head look directly to weapon aim. In virtual reality headsets (Quest 2, Quest 3, Index), forcing the player's view direction to match mechanical turret rotations induces vestibular mismatch and acute VR motion sickness. Furthermore, high-frequency hand jitter degrades micro-targeting at long engagement distances.

## Decision
We implement decoupled drone HUD reticle targeting and drift dampening in Section 6b of `src/hook/camera_hook.cpp`:
1. **Decoupled Viewport & 6DOF Aiming**:
   - The HMD orientation remains strictly decoupled from weapon azimuth and elevation, allowing natural head movement to look around the drone's cockpit/sensor sphere without steering weapons.
   - Heavy drone weapons track dominant motion controller aim ray independently.
2. **Dynamic Reticle Lead & Drift Dampening**:
   - Applies exponential lag filter ($\alpha = 0.28$) to turret aim ray:
     $\vec{P}_{\text{turret}} \leftarrow \vec{P}_{\text{turret}} + 0.28 \cdot (\vec{P}_{\text{controller}} - \vec{P}_{\text{turret}})$.
   - Eliminates hand tremor and simulates the heavy mechanical inertia of servo-driven drone cannons.
3. **Lock-On Target Confirmation**:
   - When reticle aligns with hostile targets within $3.5^\circ$ cone, emits subtle $180\text{ Hz}$ ($0.25$ amplitude) acoustic lock-on pulse.

## Consequences
- Completely eliminates simulator sickness during remote drone control and turret sequences.
- Accurately conveys heavy cybernetic turret mass and motor torque.
- Retains instant weapon responsiveness without artificial input lag.
