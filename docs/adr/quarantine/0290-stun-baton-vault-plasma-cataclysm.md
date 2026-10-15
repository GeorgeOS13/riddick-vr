# ADR 0290: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Shockwave

## Status
Accepted

## Context
Decimating maximum-security riot bastions and armored drone squads requires a catastrophic kinetic leap and dual-baton deck plunge that discharges the entire capacitor reserve in a runaway resonant cataclysm shockwave, vaporizing bulkheads and incapacitating all hostiles across 360 degrees.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm shockwave mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Cataclysmic Resonance**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -1.95\text{ m/s} \land lVel.y < -1.70\text{ m/s} \land |\Delta V_x| > 1.30\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` for airborne leap, forward drive, capacitor overload, plasma resonant wave, floor compression, girder seism overdrive, cataclysm shockwave propagation, and omnidirectional deck detonation.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) resonant cataclysm shockwave on both controllers and $160\text{ Hz}$ ($0.98$) structural girder recoil.

## Consequences
- Ultimate high-voltage AOE combat finisher.
- Seamlessly cascades with resonant seism overdrives, resonant seism waves, and harmonic deck quakes.
