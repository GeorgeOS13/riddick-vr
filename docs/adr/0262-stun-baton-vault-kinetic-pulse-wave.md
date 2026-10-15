# ADR 0262: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Pulse Discharge Wave

## Status
Accepted

## Context
Penetrating fortified security command centers guarded by multiple mechanized turrets and riot squads requires an airborne vault combined with a terminal full-capacitance discharge, blasting an omnidirectional electro-kinetic shockwave that disables turret servos and overloads riot armor.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic pulse discharge wave mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Slam with Outward Wave Flare**: Plunge both controllers downward with maximum downward acceleration on descent (`rVel.y < -1.60\text{ m/s} \land lVel.y < -1.35\text{ m/s} \land |\Delta V_x| > 0.95\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Enter` + `LMB` for airborne leap, forward drive, capacitor saturation, wave flare, and omnidirectional deck detonation.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) kinetic plasma detonation wave on both controllers and $90\text{ Hz}$ ($0.98$) structural deck recoil.

## Consequences
- Ultimate high-voltage AOE combat finisher.
- Chains naturally with omnidirectional arc pulses, deck impacts, and vault kinetic cleaves.
