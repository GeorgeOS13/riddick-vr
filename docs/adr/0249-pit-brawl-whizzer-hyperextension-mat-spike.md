# ADR 0249: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Hyperextension Mat Spike

## Status
Accepted

## Context
When facing brutal inmate pit champions who counter standard whizzers by hooking their lead leg inside Riddick's knee, arching into an overhook lever while driving the adversary's chin backward forces severe cervical hyperextension before violently slamming their head and spine into the steel floor grating.

## Decision
Implement physical whizzer hyperextension mat spike mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Hyperextension Positioning**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep overhook whizzer and cup the opponent's jaw/chin with the off-hand (`leftLocal.y > -0.05\text{ m}`, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Rotational Hyperextension Throw**: Whip dominant elbow down violently (`rVel.y < -1.40\text{ m/s}`) while thrusting the off-hand upward and laterally (`lVel.y > 0.85\text{ m/s} \land |lVel.x| > 0.90\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral pivot, crouch drop, cervical extension, and mat spike impact.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) cervical facet dislocation and cranial mat impact shockwave on dominant hand, $240\text{ Hz}$ ($0.92$) chin thrust hyperextension torque on off-hand.

## Consequences
- High-damage fatal takedown against aggressive grappling opponents.
- Chains smoothly with thoracic slams, cervical spikes, and suplexes.
