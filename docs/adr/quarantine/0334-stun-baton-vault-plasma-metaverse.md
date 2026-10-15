# ADR 0334: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Metaverse Overdrive

## Status
Accepted

## Context
Extending the omnidirectional kinetic plasma continuum beyond the omniverse tier, the metaverse overdrive synchronizes dual high-voltage stun baton capacitors into multidimensional harmonic resonance during high vault jump descent, releasing an omnidirectional kinetic plasma shockwave that sweeps the entire deck surface.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm Metaverse overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Maximum Descent Cleave**: Plunge both controllers downward with maximum descent velocity (`rVel.y < -2.50\text{ m/s} \land lVel.y < -2.25\text{ m/s} \land |\Delta V_x| > 1.85\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Space` + `LMB` + `Space` + `Enter` + `Tab` + `LMB` + `Space` + `Enter` + `Space` + `LMB` for airborne vault leap, forward propulsion, multi-phase capacitor discharge, Metaverse expansion, deck impact compression, and multi-tier plasma shockwave wave-train detonation.
- **Haptic Feedback**: $470\text{ Hz}$ ($0.98$) resonant cataclysm Metaverse overdrive shockwave on right controller and $390\text{ Hz}$ ($0.98$) structural deck Metaverse recoil on left controller.

## Consequences
- High-echelon area-of-effect tactical stun strike.
- Provides comprehensive crowd control against dense clusters of armored security personnel.
