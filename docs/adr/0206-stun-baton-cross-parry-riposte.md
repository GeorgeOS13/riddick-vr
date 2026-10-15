# ADR 0206: Security Guard Stun Baton Lateral Two-Handed Cross-Chest Kinetic Parrying Riposte

## Status
Accepted

## Context
When facing aggressive melee rushes from inmates or security personnel, an energized stun baton held in a two-handed cross-chest stance can deflect an incoming weapon laterally and immediately drive forward in a penetrative electro-kinetic riposte.

## Decision
Implement physical parrying riposte in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: In two-handed stance with crossed wrists ($dist \in [0.18, 0.35]\text{ m}, |\Delta X| > 0.20\text{ m}$, both grips $> 0.65$, dominant trigger $> 0.65$).
- **Two-Stage Gesture**:
  1. *Deflection*: Execute a sharp horizontal parry deflection sweep ($|V_x| > 0.85\text{ m/s}$), arming a $500\text{ ms}$ riposte execution window.
  2. *Riposte*: Drive a high-velocity forward linear thrust ($rVel.z < -1.25\text{ m/s}$) before timeout to discharge the counter-arc into the attacker's exposed torso.
- **Synthesized Controls**: Emits `RMB` + `A`/`D` + `W` + `Q` + `LMB` for active block, directional deflection, forward step, and high-voltage riposte.
- **Haptic Feedback**: $300\text{ Hz}$ ($0.95$) parry deflection shockwave followed by $350\text{ Hz}$ ($0.98$) electro-kinetic counter-detonation blast.

## Consequences
- Solidifies defensive depth and high-skill timing rewards for the stun baton weapon class.
- Pairs with diagonal cross-parries, defensive arc walls, and javelin thrusts.
