# ADR 0202: Security Guard Stun Baton Forward High-Arc Overhead Vault Drop Strike

## Status
Accepted

## Context
When dropping from elevated catwalks, crates, or ventilation conduits onto patrolling guards, Riddick can channel kinetic falling momentum into a lethal vertical shockwave using the energized stun baton.

## Decision
Implement physical aerial vault drop strike in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Look downward ($\text{pitch} < -25^\circ$) with dominant hand raised overhead holding energized baton ($rightLocal.y > 0.20\text{ m}$, Right Grip $> 0.60$, Right Trigger $> 0.65$).
- **Downward Plunge**: Drive dominant arm downward with high falling velocity ($rVel.y < -1.35\text{ m/s}$) to deliver a crushing vertical drop strike upon impact.
- **Synthesized Controls**: Emits `Space` + `S` + `W` + `LMB` for aerial plunge attack, kinetic conversion, and electrical stun blast.
- **Haptic Feedback**: $350\text{ Hz}$ ($0.98$) electro-kinetic detonation shockwave coupled with dual $55\text{ Hz}$ ($0.95$) landing dispersion thud.

## Consequences
- Bridges stealth vertical infiltration with devastating tactical melee finishes.
- Integrates with overhead pipe drops, vent perch takedowns, and charged skull breakers.
