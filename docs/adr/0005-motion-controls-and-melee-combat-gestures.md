# ADR 0005: Motion Controls & Melee Combat Translation

## Status
Accepted

## Context
Combat in *Escape from Butcher Bay* is divided into two distinct archetypes:
1. **Firearms:** Shotguns, assault rifles, pistols, tranquilizer guns, and heavy mech cannons.
2. **Brutal Melee & Unarmed:** Bare-knuckle boxing (jabs, hooks, uppercuts, blocks, sways), improvised shivs, and curved blades (Ulaks).

In flat gaming, firearms are centered on screen, and melee is driven by directional keys + attack button combos (e.g. Forward + Attack for a lunging stab, Left/Right + Attack for hooks, Block button to guard).

For a premier VR port, we need both playable progression and realistic physical motion handling.

## Decision
We adopt a **Two-Tier Incremental Input Model**:

### Tier 1: Core 6DOF VR with Controller Emulation (Milestone 1)
- The player views the world in full 6DOF VR through the headset.
- VR motion controllers act as high-precision gamepads:
  - Left stick: Smooth locomotion (strafing/walking).
  - Right stick: Smooth or snap turning.
  - Triggers & Grips: Fire, block, melee swing, reload, interact.
- Weapon and melee aiming is locked to the player's head gaze or forward body vector.
- **Goal:** Rapidly test entire campaign levels, checkpoints, collision, and rendering stability without being blocked by complex physical IK rigging.

### Tier 2: Tracked Motion Controls & Physical Gestures (Milestone 2)
- **Firearms Detachment:**
  - Viewmodel matrices are detached from the screen center and parented to the dominant hand's OpenXR aim pose matrix.
  - Projectile rays and bullet traces are recalculated originating from the 3D gun barrel nozzle rather than the screen center.
  - Two-handed weapon stabilization using the off-hand grip pose.
- **Melee Gestures (Punches & Ulaks):**
  - Hook into the engine's melee trigger functions.
  - Monitor velocity and acceleration vectors from the motion controllers.
  - A forward punching gesture with the right controller executes an in-game jab/cross; an upward arc executes an uppercut; bringing both controllers up near the chin triggers the guard/block state.
  - Physical contact detection with enemy hitboxes to trigger impact sound and damage.

```
          [VR Motion Controller Input]
                       │
       ┌───────────────┴───────────────┐
       ▼                               ▼
[Tier 1: Core Playable]     [Tier 2: Tracked Motion]
- Stick Movement            - 6DOF Weapon Parenting
- Button Combos             - Barrel Raycast Ballistics
- Head/Torso Aim            - Physical Velocity Swings
- Fast Milestone Validation - Guard Pose Detection
```

## Consequences
### Positive
- Players and developers can play through the entire campaign early in development using Tier 1 while Tier 2 motion features are developed.
- Physical melee boxing and shiv executions will provide unprecedented tactile immersion matching Riddick's identity.

### Negative / Challenges
- Reworking melee collision from animation-based hitboxes to spatial controller velocities requires reverse-engineering the Starbreeze combat damage resolution code.
