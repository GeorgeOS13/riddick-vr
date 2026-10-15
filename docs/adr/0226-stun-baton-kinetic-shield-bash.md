# ADR 0226: Security Guard Stun Baton Lateral Two-Handed Kinetic Shield Bash & Arc Burst

## Status
Accepted

## Context
Breaking enemy firing lines or throwing back aggressive inmates requires using the energized stun baton as a horizontal kinetic battering ram, discharging a concussive shockwave while driving opponents back.

## Decision
Implement physical two-handed stun baton shield bash in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: In two-handed horizontal braced stance ($dist \in [0.18, 0.35]\text{ m}, |\Delta X| > 0.20\text{ m}$, both grips $> 0.65$, Right Trigger $> 0.65$).
- **Forward Kinetic Bash**: Drive both arms forward explosively ($rVel.z < -1.35\text{ m/s} \land lVel.z < -1.15\text{ m/s}$).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `LMB` for forward kinetic momentum, shield bash impulse, and planar arc wave discharge.
- **Haptic Feedback**: Dual $340\text{ Hz}$ ($0.98$) planar electrostatic blast followed by dual $80\text{ Hz}$ ($0.85$) blunt kinetic recoil.

## Consequences
- Broadens crowd control options with the stun baton in close quarters.
- Integrates with cross-chest thrusts, radial disarms, and kinetic hammerfists.
