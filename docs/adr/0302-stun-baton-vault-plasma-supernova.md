# ADR 0302: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Supernova

## Status
Accepted

## Context
When facing super-heavy armored mech units and entrenched riot shock troopers, overcharging dual stun batons past critical breakdown limits during an airborne vault jump and slamming both tips into structural deck plating unleashes an omnidirectional kinetic plasma resonant cataclysm supernova, vaporizing armor and triggering a devastating electromagnetic blast across the entire sector.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm supernova mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Supernova Expansion**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -2.10\text{ m/s} \land lVel.y < -1.85\text{ m/s} \land |\Delta V_x| > 1.45\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` for leap, forward surge, capacitor overload, supernova expansion, deck compression, and catastrophic plasma detonation.
- **Haptic Feedback**: $400\text{ Hz}$ ($0.98$) resonant cataclysm supernova shockwave on right controller and $240\text{ Hz}$ ($0.98$) structural deck supernova recoil on left controller.

## Consequences
- The absolute highest-tier high-voltage AOE combat finisher.
- Serves as the ultimate culmination of the stun baton vault progression.
