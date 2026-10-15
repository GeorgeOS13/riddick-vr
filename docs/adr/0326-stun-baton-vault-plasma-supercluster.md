# ADR 0326: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Supercluster Overdrive

## Status
Accepted

## Context
When facing battalion-scale mechanized insurgencies, conducting an aerial vault jump and compressing dual stun batons into supercluster resonance before cleaving into structural deck plating detonates a massive constellation of secondary kinetic plasma nodes, clearing entire hangar decks in a blinding spherical detonation.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm Supercluster overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Maximum Descent Plunge Velocity**: Plunge both controllers downward with maximum descent velocity (`rVel.y < -2.40\text{ m/s} \land lVel.y < -2.15\text{ m/s} \land |\Delta V_x| > 1.75\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Tab` + `Space` + `LMB` + `Space` + `Enter` + `Tab` + `LMB` + `Space` + `Enter` + `Tab` for airborne leap, forward surge, capacitor overload, Supercluster expansion, deck compression, and catastrophic plasma detonation.
- **Haptic Feedback**: $450\text{ Hz}$ ($0.98$) resonant cataclysm Supercluster overdrive shockwave on right controller and $360\text{ Hz}$ ($0.98$) structural deck Supercluster recoil on left controller.

## Consequences
- The absolute highest echelon of high-voltage kinetic AOE combat finishers.
- Obliterates heavy armored targets across an expanded spherical radius of effect.
