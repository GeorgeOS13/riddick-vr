# ADR 0325: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Post Pivot-Twist Fracture

## Status
Accepted

## Context
When pinning an inmate brawl opponent against a structural post, driving a high-torque rotational pivot-twist while maintaining the thoracic hyperextension collar tie wrenches the opponent's cervical spine against the steel flange corner, delivering a devastating spinal dislocation and finish.

## Decision
Implement physical whizzer thoracic spine hyperextension cranial post pivot-twist fracture mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, both grips $> 0.65$, right grip $> 0.65$).
- **Violent Rotational Lateral-Downward Whip**: Plunge downward and whip laterally with extreme torque (`rVel.y < -2.35\text{ m/s} \land lVel.y < -2.10\text{ m/s} \land |lVel.x| > 1.00\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` + `Space` + `C` + `LMB` for structural post compression, lateral pivot torque, and cervical dislocation execution.
- **Haptic Feedback**: $450\text{ Hz}$ ($0.98$) cervical vertebra fracture shockwave on dominant hand, $370\text{ Hz}$ ($0.95$) whizzer collar tie drag on off-hand.

## Consequences
- Traumatic, brutal environmental execution.
- Advances pit fighting close-quarter lethal dominance against heavyweight opponents.
