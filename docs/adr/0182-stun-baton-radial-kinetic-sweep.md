# ADR 0182: Security Guard Stun Baton Radial Kinetic Sweep & Electrical Stun Ring

## Status
Accepted

## Context
When surrounded by multiple inmates or security guards, holding the stun baton with both hands and executing a wide, horizontal centrifugal spin sweeps through enemy lines, releasing a circular perimeter electrical arc discharge that stuns and repels all adjacent threats.

## Decision
Implement physical radial stun sweep and electrical discharge in `src/hook/camera_hook.cpp`:
- **Stance & Readiness**: In two-handed stance ($dist < 0.22\text{ m}$) with stun baton energized ($Grip > 0.60, Trigger > 0.65$).
- **Horizontal Centrifugal Sweep**: Sweep both arms horizontally in a wide radial arc across the body ($|V_{x, right}| > 1.35\text{ m/s} \land |V_{x, left}| > 1.10\text{ m/s}$ with both controllers moving in the same direction, $\text{sgn}(V_x) > 0$ or $< 0$).
- **Synthesized Controls**: Emits `A`/`D` (directional sweep) + `Space` + `Q` + `LMB` for full perimeter arc discharge and kinetic knockback.
- **Haptic Feedback**: Continuous $200\text{ Hz}$ ($0.65$) centrifugal ionization hum, terminating in a dual-controller $310\text{ Hz}$ ($0.98$) radial discharge blast.

## Consequences
- Provides vital crowd-control capability when flanked or swarmed by multiple attackers.
- Complements existing directional stun baton tactics (javelin thrust, skull breaker, sweep disarm, cross-block arc burst).
