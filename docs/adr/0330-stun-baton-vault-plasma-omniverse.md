# ADR 0330: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Omniverse Overdrive

## Status
Accepted

## Context
The ultimate manifestation of the stun baton kinetic arc system. At maximum apex trajectory during a high vault jump, saturating dual batons into omniverse resonance and driving both tips downward into the deck detonates a multi-planar kinetic plasma explosion that annihilates everything in a catastrophic spherical expansion.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm Omniverse overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Maximum Descent Cleave**: Plunge both controllers downward with maximum descent velocity (`rVel.y < -2.45\text{ m/s} \land lVel.y < -2.20\text{ m/s} \land |\Delta V_x| > 1.80\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Tab` + `Space` + `LMB` + `Space` + `Enter` + `Tab` + `LMB` + `Space` + `Enter` + `Tab` + `Space` + `LMB` for airborne leap, forward surge, capacitor overload, Omniverse expansion, deck compression, and cosmic multi-planar plasma detonation.
- **Haptic Feedback**: $460\text{ Hz}$ ($0.98$) resonant cataclysm Omniverse overdrive shockwave on right controller and $380\text{ Hz}$ ($0.98$) structural deck Omniverse recoil on left controller.

## Consequences
- The absolute highest echelon of high-voltage kinetic AOE combat finishers.
- Obliterates heavy armored targets across an expanded spherical radius of effect.
