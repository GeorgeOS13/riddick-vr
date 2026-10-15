# ADR 0322: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Cosmic Dawn Overdrive

## Status
Accepted

## Context
At the pinnacle of stun baton combat capability, saturating twin batons with resonant dielectric overcharge during maximum vault trajectory and plunging both tips with terminal descent velocity into deck armor triggers the Cosmic Dawn overdrive—an expanding celestial wavefront of ionized plasma that vaporizes all adversaries and shatters structural bulkheads across the entire sector.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm Cosmic Dawn overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Maximum Descent Plunge Velocity**: Plunge both controllers downward with maximum descent velocity (`rVel.y < -2.35\text{ m/s} \land lVel.y < -2.10\text{ m/s} \land |\Delta V_x| > 1.70\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Tab` + `Space` + `LMB` + `Space` + `Enter` + `Tab` + `LMB` for airborne leap, forward surge, capacitor overload, Cosmic Dawn expansion, deck compression, and catastrophic plasma detonation.
- **Haptic Feedback**: $440\text{ Hz}$ ($0.98$) resonant cataclysm Cosmic Dawn overdrive shockwave on right controller and $340\text{ Hz}$ ($0.98$) structural deck Cosmic Dawn recoil on left controller.

## Consequences
- The absolute highest echelon of high-voltage kinetic AOE combat finishers.
- Obliterates heavy armored targets across an expanded spherical radius of effect.
