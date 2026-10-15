# ADR 0270: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Shockwave Overload

## Status
Accepted

## Context
Overwhelming fortified security defense perimeters guarded by automated heavy turrets and riot wardens requires a catastrophic kinetic breach. Vaulting high above incoming fire and driving both overcharged stun batons into the deck plate triggers a complete capacitive plasma overload, vaporizing nearby armor and neutralizing all electronic systems across 360 degrees.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma shockwave overload mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Radial Overload Wave**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -1.70\text{ m/s} \land lVel.y < -1.45\text{ m/s} \land |\Delta V_x| > 1.05\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `LMB` for airborne leap, forward drive, capacitor saturation, wave flare, crouch deck compression, and omnidirectional deck detonation.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) plasma overload detonation wave on both controllers and $110\text{ Hz}$ ($0.98$) structural deck recoil.

## Consequences
- Ultimate high-voltage AOE combat finisher.
- Seamlessly cascades with plasma cleave waves, kinetic pulse waves, and vault kinetic cleaves.
