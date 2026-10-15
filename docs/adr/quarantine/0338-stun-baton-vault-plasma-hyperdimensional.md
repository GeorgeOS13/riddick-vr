# ADR 0338: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Hyperdimensional Overdrive

## Status
Accepted

## Context
Pushing the kinetic plasma discharge envelope beyond the metaverse tier (ADR 0334), hyperdimensional overdrive modulates the dual stun baton capacitors at ultra-high harmonic frequencies during vault descent, triggering an omnidirectional spherical plasma wave that sweeps all surface terrain and structural cover.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm Hyperdimensional overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Maximum Descent Cleave**: Plunge both controllers downward with maximum descent velocity (`rVel.y < -2.55\text{ m/s} \land lVel.y < -2.30\text{ m/s} \land |\Delta V_x| > 1.90\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Space` + `LMB` + `Space` + `Enter` + `Tab` + `LMB` + `Space` + `Enter` + `Space` + `LMB` + `Space` + `Enter` + `Tab` + `LMB` for airborne leap, forward propulsion, hyperdimensional capacitor saturation, deck impact compression, and multi-planar shockwave detonation.
- **Haptic Feedback**: $480\text{ Hz}$ ($0.98$) resonant cataclysm Hyperdimensional overdrive shockwave on right controller and $400\text{ Hz}$ ($0.98$) structural deck Hyperdimensional recoil on left controller.

## Consequences
- The absolute highest echelon of area-of-effect tactical stun detonations.
- Vaporizes clustered adversary squads across all elevations and cover positions.
