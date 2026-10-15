# ADR 0214: Security Guard Stun Baton Charged High-Velocity Forward Radial Thrust Disarm

## Status
Accepted

## Context
When facing adversaries brandishing firearms or heavy clubs, a rapid torque rotation of an energized stun baton can strip the weapon from an opponent's hands and transition directly into a high-voltage penetrative core thrust.

## Decision
Implement physical radial disarm thrust in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: In two-handed stance with stun baton energized ($dist < 0.24\text{ m}$, both grips $> 0.65$, Right Trigger $> 0.65$).
- **Two-Stage Gesture**:
  1. *Torque Disarm*: Rotate wrists in an aggressive opposing lateral torque ($|\Delta V_x| > 1.15\text{ m/s}$), stripping enemy weapons and arming a $500\text{ ms}$ thrust window.
  2. *Penetrative Thrust*: Drive dominant hand forward with extreme velocity ($rVel.z < -1.35\text{ m/s}$) to discharge high-voltage capacitor into enemy torso.
- **Synthesized Controls**: Emits `RMB` + `A`/`D` + `W` + `E` + `LMB` for active guard, rotational disarm, forward step, weapon pickup, and kinetic thrust.
- **Haptic Feedback**: $310\text{ Hz}$ ($0.95$) torque disarm snap followed by $350\text{ Hz}$ ($0.98$) penetrative arc detonation blast.

## Consequences
- Solidifies defensive disarm and counter-attack flow for the stun baton.
- Pairs with cross-parry ripostes, javelin lunges, and overcharge detonations.
