# ADR 0337: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Chokehold Finisher

## Status
Accepted

## Context
Following a structural post lever-pin against an adversary (ADR 0333), wrapping the lead forearm across their carotid triangle while jamming their occiput against the steel post flange transforms the post into a rigid fulcrum, locking an inescapable vascular compression choke that immediately finishes the fight.

## Decision
Implement physical whizzer thoracic spine cranial post fulcrum leverage chokehold finisher mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, both grips $> 0.65$, right grip $> 0.65$).
- **Violent Fulcrum Compression Plunge**: Plunge downward and drive opposing lateral torque (`rVel.y < -2.50\text{ m/s} \land lVel.y < -2.25\text{ m/s} \land |lVel.x| > 1.10\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` + `Space` + `C` + `LMB` + `Space` + `Q` + `Tab` + `LMB` for post corner entrapment, fulcrum vascular squeeze, and ground-pin submission.
- **Haptic Feedback**: $480\text{ Hz}$ ($0.98$) fulcrum leverage and structural compression shockwave on dominant hand, $400\text{ Hz}$ ($0.95$) whizzer collar tie drag torque on off-hand.

## Consequences
- High-efficiency environmental submission maneuver.
- Neutralizes high-health inmate and guard targets using architectural corner geometry.
