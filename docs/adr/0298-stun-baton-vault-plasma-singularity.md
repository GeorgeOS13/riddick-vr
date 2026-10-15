# ADR 0298: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Singularity

## Status
Accepted

## Context
In maximum-security containment breaches, overloading dual stun batons at peak airborne vault trajectory generates an implosive kinetic plasma singularity that collapses nearby electromagnetic fields before detonating a catastrophic 360-degree shockwave on deck impact, vaporizing hostile shields and cybernetic drones.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm singularity mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Plasma Singularity**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -2.05\text{ m/s} \land lVel.y < -1.80\text{ m/s} \land |\Delta V_x| > 1.40\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Q` + `Tab` for airborne leap, forward surge, capacitor overload, singularity collapse, deck compression, and catastrophic plasma detonation.
- **Haptic Feedback**: $390\text{ Hz}$ ($0.98$) resonant cataclysm singularity shockwave on right controller and $220\text{ Hz}$ ($0.98$) structural deck singularity recoil on left controller.

## Consequences
- Ultimate apex-tier high-voltage AOE combat finisher.
- Naturally culminates the stun baton vault sequence to maximum destructive capacity.
