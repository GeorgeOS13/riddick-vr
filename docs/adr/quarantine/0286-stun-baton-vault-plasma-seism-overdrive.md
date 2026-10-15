# ADR 0286: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Seism Overdrive

## Status
Accepted

## Context
Obliterating fortified perimeter command centers guarded by automated heavy turrets and armored riot wardens requires a maximum airborne vault and dual-baton deck plunge that discharges the full capacitor bank in a high-harmonic resonant seism overdrive, tearing through deck plating and incapacitating all electronic systems and personnel across 360 degrees.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant seism overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Seismic Resonance Overdrive**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -1.90\text{ m/s} \land lVel.y < -1.65\text{ m/s} \land |\Delta V_x| > 1.25\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `LMB` for airborne leap, forward drive, capacitor overload, plasma resonant wave, floor compression, girder seism overdrive, and omnidirectional deck detonation.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) resonant seism overdrive shockwave on both controllers and $150\text{ Hz}$ ($0.98$) structural girder recoil.

## Consequences
- Ultimate high-voltage AOE combat finisher.
- Seamlessly cascades with resonant seism waves, harmonic deck quakes, and resonant detonations.
