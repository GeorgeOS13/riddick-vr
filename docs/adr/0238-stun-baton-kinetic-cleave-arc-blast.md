# ADR 0238: Security Guard Stun Baton Forward Two-Handed Kinetic Cleave & Arc Discharge Blast

## Status
Accepted

## Context
Neutralizing heavily shielded security forces demands combining vertical kinetic cleave momentum with forward explosive lunging. Driving the energized stun baton downward and forward in a unified two-handed stroke discharges a frontal high-voltage shockwave that shatters shields and arcs between adjacent targets.

## Decision
Implement physical two-handed kinetic cleave and arc blast in `src/hook/camera_hook.cpp`:
- **Overhead High Stance**: Both controllers held high in front of head (`handsDistSledge < 0.28\text{ m}`, `rightLocal.y > 0.16\text{ m}`, `leftLocal.y > 0.08\text{ m}`, left grip $> 0.65$, right grip $> 0.65$, right trigger $> 0.65$).
- **Diagonal Forward-Downward Cleave**: Snap both controllers downward and forward simultaneously (`rVel.y < -1.30\text{ m/s} \land lVel.y < -1.05\text{ m/s} \land rVel.z < -1.00\text{ m/s}`).
- **Synthesized Controls**: Emits `S` + `W` + `Q` + `Space` + `LMB` for stance drop, forward lunge, arc overcharge, jump compression, and devastating impact.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) frontal arc discharge detonation on the right controller and $220\text{ Hz}$ ($0.90$) two-handed shaft recoil on the left controller.

## Consequences
- High-impact offensive combination breaking multiple enemy guards.
- Bridges the gap between grounding thrusts, kinetic shield bashes, and hammerfist strikes.
