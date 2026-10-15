# ADR 0230: Security Guard Stun Baton Forward High-Voltage Grounding Thrust & Arc Discharge Ring

## Status
Accepted

## Context
Engaging multiple armored adversaries in confined corridors requires completing an electro-kinetic circuit between the floor deck and the target. Sinking into a low-line stance and thrusting the energized stun baton horizontally channels high-amperage current through the target, culminating in an expanding electrostatic ground ring.

## Decision
Implement physical grounding stun baton thrust in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: In low-line crouched stance ($leftLocal.y < -0.25\text{ m} \lor s\_isPhysicallyCrouched$), hold two-handed grip with stun baton energized ($dist < 0.24\text{ m}$, both grips $> 0.65$, Right Trigger $> 0.65$).
- **Linear Grounding Thrust**: Drive both hands forward with high velocity ($rVel.z < -1.35\text{ m/s} \land lVel.z < -1.10\text{ m/s}$).
- **Synthesized Controls**: Emits `C` + `W` + `Q` + `Space` + `LMB` for low-line grounding stance, linear arc penetration, and circuit discharge.
- **Haptic Feedback**: $350\text{ Hz}$ ($0.98$) closed-circuit dielectric breakdown shockwave and $60\text{ Hz}$ ($0.90$) floor conduit surge.

## Consequences
- Expands defensive and offensive stun baton tactics from crouch/cover.
- Integrates with kinetic shield bashes, javelin vault thrusts, and taser kneels.
