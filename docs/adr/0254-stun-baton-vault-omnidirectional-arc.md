# ADR 0254: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Deck Arc Dispersion

## Status
Accepted

## Context
When surrounded by multiple squads of armored guards in central junctions, executing an airborne vault jump and thrusting the energized stun baton downward with an expansive horizontal wrist flare detonates a 360-degree high-voltage dispersion wave that grounds surrounding adversaries.

## Decision
Implement physical stun baton vault jump omnidirectional deck arc dispersion mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Downward Plunge with Lateral Arc Spread**: Plunge both controllers downward while flaring wrists apart on descent (`rVel.y < -1.45\text{ m/s} \land lVel.y < -1.20\text{ m/s} \land |\Delta V_x| > 0.80\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `LMB` for airborne vault, forward trajectory, capacitive discharge, omnidirectional arc flare, and deck ground detonation.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) radial dielectric breakdown wave on both controllers and $75\text{ Hz}$ ($0.95$) landing seismic shockwave.

## Consequences
- Premier crowd-control aerial finisher for stun baton combat.
- Complements deck impacts, kinetic cleaves, and grounding thrusts.
