# ADR 0190: Security Guard Stun Baton Forward High-Voltage Discharge Jab & Taser Trip

## Status
Accepted

## Context
Combining a high-speed linear jab with an immediate low sweep using the stun baton allows an operator to disrupt an enemy's posture with a direct chest shock before catching their lead leg with the electrified tip to send them crashing to the ground.

## Decision
Implement physical stun baton jab-to-sweep combination in `src/hook/camera_hook.cpp`:
- **Initial Shock Jab**: Deliver a high-speed linear jab ($rVel.z < -1.25\text{ m/s}, Right Grip > 0.60, Right Trigger > 0.65$).
- **Low Sweep Trip Follow-Through**: Follow within $500\text{ ms}$ by a downward-lateral leg sweep ($rVel.y < -1.10\text{ m/s} \land |rVel.x| > 0.80\text{ m/s}$).
- **Synthesized Controls**: Emits `W` + `S` + `C` + `LMB` for jab impact, posture breakdown, and trip takedown.
- **Haptic Feedback**: $310\text{ Hz}$ ($0.98$) electrical contact discharge, followed by $180\text{ Hz}$ ($0.90$) low-line sweep trip impact.

## Consequences
- Fluid two-stage combination attack rewarding dynamic controller follow-through.
- Bridges striking and takedown states in the stun baton weapon moveset.
