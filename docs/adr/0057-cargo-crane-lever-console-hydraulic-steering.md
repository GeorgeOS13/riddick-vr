# ADR 0057: Heavy Automated Cargo Crane Lever Console Steering

## Status
Accepted / Implemented

## Context
In Butcher Bay's Cargo Facility and Dark Athena's hangar deck bays, players must operate overhead gantry cranes, container stackers, and heavy industrial freight lifters to rearrange cargo containers, clear obstructed exits, or squash patrolling security drones. Flat-screen interactions map crane operations to generic keyboard controls. In VR, grasping physical crane console levers and pushing/pulling them with tangible hydraulic vibration provides authentic industrial immersion.

## Decision
We implement cargo crane console lever manipulation in Section 6 of `src/hook/camera_hook.cpp`:
1. **Console Control Zone & Lever Contact**:
   - Controller positioned at the crane control deck:
     $Z_{local} \in [-0.40, -0.15]\text{ m}$, $Y_{local} \in [-0.35, -0.05]\text{ m}$.
   - Hand grip engaged ($> 0.50$).
2. **Boom Translation & Trolley Traverse**:
   - Longitudinal displacement / lever push ($V_z < -0.30\text{ m/s}$) drives boom forward (`W`), pull ($V_z > +0.30\text{ m/s}$) drives boom backward (`S`).
   - Lateral lever throw ($V_x < -0.30\text{ m/s}$) drives trolley left (`A`), throw ($V_x > +0.30\text{ m/s}$) drives trolley right (`D`).
3. **Hydraulic Vibration Haptics**:
   - Continuous low-frequency $50\text{ Hz}$ ($0.35$ amplitude) motor and cable tension shudder emitted every $200\text{ ms}$ while levers are actively displaced.

## Consequences
- Elevates industrial machinery manipulation to a tactile room-scale VR experience.
- Cleanly decoupled from player locomotion (crane console lock prevents walking during operation).
- Low frequency ($50\text{ Hz}$) communicates heavy tons of steel suspended in motion.
