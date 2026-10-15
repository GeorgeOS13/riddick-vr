# ADR 0329: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Post Pivot-Twist Deck Slam

## Status
Accepted

## Context
When an opponent is pinned against a structural post with cervical hyperextension, following the pivot-twist with a violent downward body-drop slam wrenches their head off the post and spikes their torso flat into the diamond deck grating, finishing the brawl with complete trauma.

## Decision
Implement physical whizzer thoracic spine hyperextension cranial post pivot-twist deck slam mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, both grips $> 0.65$, right grip $> 0.65$).
- **Violent Rotational Lateral-Downward Body Drop**: Plunge downward and whip laterally with maximum momentum (`rVel.y < -2.40\text{ m/s} \land lVel.y < -2.15\text{ m/s} \land |lVel.x| > 1.00\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` + `Space` + `C` + `Space` + `LMB` for post pivot compression, lateral whip, and concussive deck slam finish.
- **Haptic Feedback**: $460\text{ Hz}$ ($0.98$) concussive deck impact and spinal fracture shockwave on dominant hand, $380\text{ Hz}$ ($0.95$) whizzer collar tie drag on off-hand.

## Consequences
- Traumatic, brutal environmental execution.
- Advances pit fighting close-quarter lethal dominance against heavyweight opponents.
