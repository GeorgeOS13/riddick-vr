# ADR 0346: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Interdimensional Overdrive

## Status
Accepted

## Context
Extending the tactical stun continuum beyond transdimensional thresholds (ADR 0342), interdimensional overdrive saturates the dual stun baton primary resonant tank into full interdimensional plasma cascade during maximum vault descent, unleashing an expanding spherical plasma detonation upon deck impact that sweeps all platforms, catwalks, and vertical tiers.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm Interdimensional overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Maximum Descent Cleave**: Plunge both controllers downward with maximum descent velocity (`rVel.y < -2.65\text{ m/s} \land lVel.y < -2.40\text{ m/s} \land |\Delta V_x| > 2.00\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Space` + `LMB` + `Space` + `Enter` + `Tab` + `LMB` + `Space` + `Enter` + `Space` + `LMB` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `Space` + `Enter` + `LMB` for airborne vault leap, forward surge, interdimensional coil cascade, deck impact compression, and multi-tier plasma shockwave wave-train detonation.
- **Haptic Feedback**: $500\text{ Hz}$ ($0.98$) resonant cataclysm Interdimensional overdrive shockwave on right controller and $420\text{ Hz}$ ($0.98$) structural deck Interdimensional recoil on left controller.

## Consequences
- The absolute highest echelon of area-of-effect tactical stun detonations.
- Eliminates heavily armored enemy battalions across complex multi-level arenas.
