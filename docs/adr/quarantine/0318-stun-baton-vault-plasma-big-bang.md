# ADR 0318: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Big Bang Overdrive

## Status
Accepted

## Context
In ultimate high-voltage kinetic combat, overloading twin stun batons at peak vault trajectory with hyper-dense electrostatic compression and driving both tips with full gravitational acceleration into structural deck plating detonates a resonant cataclysm Big Bang overdrive, unleashing a blinding omnidirectional kinetic plasma blast that disintegrates multiple phalanxes in an expanding spherical firestorm.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm Big Bang overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Big Bang Detonation**: Plunge both controllers downward with maximum descent velocity (`rVel.y < -2.30\text{ m/s} \land lVel.y < -2.05\text{ m/s} \land |\Delta V_x| > 1.65\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Tab` + `Space` + `LMB` + `Space` + `Enter` for airborne leap, forward surge, capacitor overload, Big Bang expansion, deck compression, and cosmic plasma detonation.
- **Haptic Feedback**: $430\text{ Hz}$ ($0.98$) resonant cataclysm Big Bang overdrive shockwave on right controller and $320\text{ Hz}$ ($0.98$) structural deck Big Bang recoil on left controller.

## Consequences
- The absolute pinnacle of the high-voltage kinetic stun baton progression.
- Annihilates all hostiles and obstacles in an expanded spherical blast radius.
