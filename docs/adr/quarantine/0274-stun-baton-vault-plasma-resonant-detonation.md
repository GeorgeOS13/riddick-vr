# ADR 0274: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Detonation

## Status
Accepted

## Context
Neutralizing deep containment defensive bastions fortified by automated turret batteries and heavy riot wardens requires a catastrophic kinetic leap and dual-baton deck plunge that triggers harmonic plasma resonance across the entire floor substrate, destroying all electronics and shattering armor plating.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant detonation mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Harmonic Wave Detonation**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -1.75\text{ m/s} \land lVel.y < -1.50\text{ m/s} \land |\Delta V_x| > 1.10\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `LMB` for airborne leap, forward drive, capacitor overload, plasma resonant wave, crouch floor compression, and omnidirectional deck detonation.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) harmonic plasma detonation wave on both controllers and $120\text{ Hz}$ ($0.98$) structural deck recoil.

## Consequences
- Ultimate high-voltage AOE combat finisher.
- Seamlessly cascades with plasma overload waves, cleave waves, and kinetic pulse waves.
