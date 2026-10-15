# ADR 0278: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Harmonic Deck Quake

## Status
Accepted

## Context
Decimating heavily entrenched riot defense bastions requires a supreme kinetic vault and dual-baton deck plunge that discharges the entire capacitor bank directly into structural deck beams, propagating a harmonic seismic plasma deck quake that shatters bulkheads and annihilates riot wardens.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma harmonic deck quake mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Harmonic Quake Detonation**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -1.80\text{ m/s} \land lVel.y < -1.55\text{ m/s} \land |\Delta V_x| > 1.15\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `LMB` for airborne leap, forward drive, capacitor overload, plasma resonant wave, floor compression, deck quake propagation, and omnidirectional deck detonation.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) harmonic plasma quake wave on both controllers and $130\text{ Hz}$ ($0.98$) structural deck recoil.

## Consequences
- Ultimate high-voltage AOE combat finisher.
- Seamlessly cascades with resonant detonations, overload waves, and cleave waves.
