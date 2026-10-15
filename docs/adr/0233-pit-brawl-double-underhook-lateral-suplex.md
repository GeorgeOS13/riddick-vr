# ADR 0233: Inmate Pit Brawl Double Underhook Lateral Suplex Arch & Mat Spike

## Status
Accepted

## Context
When an inmate brawl opponent entrenches in a defensive forward clinch, wrapping double underhooks allows Riddick to break their posture. Arching the back and exploding into a lateral roll tosses the opponent over the hip, driving their cervical spine and shoulder into the steel grating.

## Decision
Implement physical double underhook lateral suplex mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Underhook Positioning**: Both fists tight inside opponent's frame (`handsDistSledge < 0.28\text{ m}`, `leftLocal.y > -0.20\text{ m}`, `rightLocal.y > -0.20\text{ m}`, left grip $> 0.65$, right grip $> 0.65$).
- **Arch & Lateral Roll**: Drive both arms upward in a bridging heave (`rVel.y > 0.70\text{ m/s} \land lVel.y > 0.70\text{ m/s}`) combined with strong lateral torque (`|rVel.x| > 0.85\text{ m/s} \lor |lVel.x| > 0.85\text{ m/s}`).
- **Lateral Roll Direction**: Determine suplex direction from lateral velocity vector (`rVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral pivot step, crouch leverage, hip elevation, and violent mat impact.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) spine compression and mat impact shockwave on the dominant hand, $190\text{ Hz}$ ($0.92$) double underhook torso lift torque on the off hand.

## Consequences
- High-damage tactical throw from sustained clinches.
- Smoothly complements collar tie whizzers, hip tosses, and Thai plum knee strikes.
