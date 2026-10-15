# ADR 0203: Dark Athena EVA Solar Collector Photovoltaic Array Tension Guy-Wire Rigging Turnbuckle Locking Nut Strike

## Status
Accepted

## Context
High-tension guy-wires stabilizing the Dark Athena exterior photovoltaic arrays are subject to cyclical micro-vibrations from orbital station-keeping thrusters. Riddick locks the tensioned turnbuckles by striking the hexagonal lock nut with a spanner or mallet to stake the threads.

## Decision
Implement physical two-handed turnbuckle lock nut staking in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position off-hand on the turnbuckle body ($Z \in [-0.42, -0.15]\text{ m}, Y \in [-0.20, 0.20]\text{ m}$, Left Grip $> 0.55$) and dominant hand above holding mallet/tool ($dist < 0.22\text{ m}$, Right Grip $> 0.60$).
- **Impact Strike**: Deliver a downward hammer blow ($rVel.y < -1.10\text{ m/s}$) to stake the locking nut against the turnbuckle sleeve.
- **Synthesized Controls**: Emits `E` + `W` + `Space` + `Tab` for tension verification, lock nut seating, and thread staking.
- **Haptic Feedback**: $260\text{ Hz}$ ($0.98$) locking nut impact detent snap and $140\text{ Hz}$ ($0.60$) high-tension wire resonance chime.

## Consequences
- Solidifies structural exterior spacewalk maintenance tasks.
- Integrates with guy-wire turnbuckle wrenches, articulation locks, and wing jettison bolts.
