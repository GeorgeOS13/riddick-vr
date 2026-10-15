# ADR 0242: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Ground Arc Disruption

## Status
Accepted

## Context
Breaching barricaded security checkpoints requires leaping over incoming gunfire and driving the energized stun baton directly into the steel floor upon landing, propagating an expanding 360-degree ground arc ring that incapacitates grouped guards.

## Decision
Implement physical stun baton vault jump ground arc disruption mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Vault Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.24\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Downward Deck Plunge on Descent**: As descent begins, drive both controllers violently downward into the floor (`rVel.y < -1.40\text{ m/s} \land lVel.y < -1.15\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `C` + `LMB` for aerial vault, forward trajectory, arc activation, ground plant crouch, and radial shockwave discharge.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) dielectric deck breakdown on the right controller and $70\text{ Hz}$ ($0.95$) landing seismic shockwave on both controllers.

## Consequences
- Spectacular gap-closing aerial combat maneuver from sprint or jump.
- Expands upon grounded grounding thrusts, kinetic cleaves, and shield bashes.
