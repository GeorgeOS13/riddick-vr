# ADR 0266: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Cleave Wave

## Status
Accepted

## Context
Breaching fortified security containment sectors occupied by mechanized heavy riot guards requires a devastating kinetic entry. Executing a two-handed vault jump and violently driving both overcharged batons down into the deck plate discharges full capacitive energy in an expanding radial plasma cleave wave, shattering armor and incapacitating all hostiles within a wide radius.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma cleave wave mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Radial Plasma Flare**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -1.65\text{ m/s} \land lVel.y < -1.40\text{ m/s} \land |\Delta V_x| > 1.00\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `LMB` for airborne leap, forward drive, capacitor saturation, wave flare, and omnidirectional deck detonation.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) kinetic plasma cleave wave on both controllers and $100\text{ Hz}$ ($0.98$) structural deck recoil.

## Consequences
- Ultimate high-voltage AOE combat finisher.
- Seamlessly cascades with kinetic pulse waves, deck impacts, and vault kinetic cleaves.
