# ADR 0198: Security Guard Stun Baton Forward Two-Handed Cross-Chest Block & Arc Thrust Disruption

## Status
Accepted

## Context
Absorbing high-impact blunt blows while keeping the stun baton energized allows an operative to maintain a horizontal cross-chest guard, catch the impact, and immediately propel both hands forward to channel the stored kinetic and electrical energy back into the assailant.

## Decision
Implement physical two-handed cross-chest block to arc thrust disruption in `src/hook/camera_hook.cpp`:
- **Cross-Chest Guard Hold**: In horizontal cross-chest stance ($dist \in [0.20, 0.38]\text{ m}, Y \in [-0.15, 0.15]\text{ m}, Z \in [-0.30, -0.10]\text{ m}$, both grips $> 0.65$, both triggers $> 0.65$) held for $> 300\text{ ms}$.
- **Explosive Arc Thrust**: Propel both hands forward explosively ($rVel.z < -1.25\text{ m/s} \land lVel.z < -1.05\text{ m/s}$) to discharge the stored electrostatic kinetic wave.
- **Synthesized Controls**: Emits `RMB` + `W` + `Space` + `Q` + `LMB` for sustained guard block, counter-step, and high-voltage arc disruption.
- **Haptic Feedback**: Continuous $240\text{ Hz}$ ($0.65$) defensive barrier hum, detonating into dual $340\text{ Hz}$ ($0.98$) arc blast shockwaves.

## Consequences
- Rewards timed defensive positioning with high-impact counter-offensive knockback.
- Enriches tactical options against berserker inmates and armored heavy guards.
