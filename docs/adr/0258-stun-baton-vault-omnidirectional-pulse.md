# ADR 0258: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Deck Arc Pulse

## Status
Accepted

## Context
When dropping from high elevated walkways into security enclosures packed with armored drones and riot guards, channeling maximum capacitive energy during an aerial vault jump and plunging the stun baton downward with a secondary pulse flare generates a double-frequency shockwave that overloads drone electronics and grounds shields.

## Decision
Implement physical stun baton vault jump omnidirectional deck arc pulse mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Downward Plunge with Double Wrist Flare**: Plunge both controllers downward with explosive downward velocity on descent (`rVel.y < -1.55\text{ m/s} \land lVel.y < -1.30\text{ m/s} \land |\Delta V_x| > 0.90\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `LMB` for airborne vault, forward trajectory, capacitive saturation, double pulse discharge, and omnidirectional deck detonation.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) secondary electrostatic pulse blast on both controllers and $80\text{ Hz}$ ($0.98$) seismic deck concussion.

## Consequences
- Top-tier crowd-control aerial finisher neutralizing both ground infantry and cybernetic drones.
- Culminates the high-voltage airborne vault combat tree.
