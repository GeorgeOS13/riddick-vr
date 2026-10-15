# ADR 0342: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Transdimensional Overdrive

## Status
Accepted

## Context
Pushing beyond hyperdimensional limits (ADR 0338), transdimensional overdrive drives the dual stun batons' induction resonant tank into avalanche breakdown during vault jump descent, releasing an omnidirectional kinetic plasma detonation that propagates through all environmental obstacles and armor plating.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm Transdimensional overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Maximum Descent Cleave**: Plunge both controllers downward with maximum descent velocity (`rVel.y < -2.60\text{ m/s} \land lVel.y < -2.35\text{ m/s} \land |\Delta V_x| > 1.95\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Space` + `LMB` + `Space` + `Enter` + `Tab` + `LMB` + `Space` + `Enter` + `Space` + `LMB` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` for airborne leap, forward propulsion, avalanche coil saturation, transdimensional shockwave detonation, and deck impact compression.
- **Haptic Feedback**: $490\text{ Hz}$ ($0.98$) resonant cataclysm Transdimensional overdrive shockwave on right controller and $410\text{ Hz}$ ($0.98$) structural deck Transdimensional recoil on left controller.

## Consequences
- The absolute highest echelon of area-of-effect tactical stun detonations.
- Vaporizes clustered adversary squads across all elevations, angles, and physical cover.
