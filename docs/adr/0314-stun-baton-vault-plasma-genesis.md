# ADR 0314: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Genesis Overdrive

## Status
Accepted

## Context
In apex-level containment breaches against super-heavy mechanical walkers and entrenched riot phalanxes, overloading dual stun batons at peak vault trajectory and driving both tips with full gravitational acceleration into structural deck plating detonates a resonant cataclysm genesis overdrive, unleashing an omnidirectional kinetic plasma shockwave that disintegrates heavy armor and electrifies surrounding sectors in an expanding spherical wave.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm genesis overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Genesis Overdrive**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -2.25\text{ m/s} \land lVel.y < -2.00\text{ m/s} \land |\Delta V_x| > 1.60\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Tab` + `Space` + `LMB` for airborne leap, forward surge, capacitor overload, genesis expansion, deck compression, and catastrophic plasma detonation.
- **Haptic Feedback**: $420\text{ Hz}$ ($0.98$) resonant cataclysm genesis overdrive shockwave on right controller and $300\text{ Hz}$ ($0.98$) structural deck genesis recoil on left controller.

## Consequences
- The absolute highest echelon of high-voltage kinetic AOE combat finishers.
- Vaporizes heavy armored targets across a massive spherical area of effect.
