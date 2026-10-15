# ADR 0257: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Ground Dislocation

## Status
Accepted

## Context
When an inmate brawl opponent attempts to roll through a whizzer takedown to avoid head trauma, maintaining an overhook arm trap while driving their trapped shoulder downward against the steel grating wrenches the glenohumeral joint out of its socket while compressing cervical vertebrae.

## Decision
Implement physical whizzer ground dislocation mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Positioning**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep overhook whizzer and collar tie (`leftLocal.y` between $-0.30\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Shoulder Wrench**: Drive dominant forearm downward with extreme velocity (`rVel.y < -1.50\text{ m/s}`) while hauling off-hand laterally and downward (`lVel.y < -1.20\text{ m/s} \land |lVel.x| > 0.85\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral pivot, crouch drop, and shoulder capsule dislocation slam.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) shoulder socket fracture and vertebral crack on dominant hand, $230\text{ Hz}$ ($0.92$) overhook torque strain on off-hand.

## Consequences
- Lethal submission takedown against heavy brawlers.
- Completes the double collar tie whizzer grappling progression.
