# ADR 0333: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Lever-Pin Lock Takedown

## Status
Accepted

## Context
Following a structural post pivot-twist against an opponent, driving an overhook collar-tie lever against their upper cervical line and wedging their shoulder against the vertical support post creates an inescapable fulcrum lock that forces their posture down into a grounded deck pin.

## Decision
Implement physical whizzer thoracic spine cranial post lever-pin lock takedown mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, both grips $> 0.65$, right grip $> 0.65$).
- **Violent Rotational Reverse-Lever Plunge**: Plunge downward and drive opposing lateral torque (`rVel.y < -2.45\text{ m/s} \land lVel.y < -2.20\text{ m/s} \land |lVel.x| > 1.05\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` + `Space` + `C` + `LMB` + `Space` + `Q` + `LMB` for post lever compression, fulcrum lock twist, and deck pinning execution.
- **Haptic Feedback**: $470\text{ Hz}$ ($0.98$) fulcrum leverage and structural deck impact shockwave on dominant hand, $390\text{ Hz}$ ($0.95$) whizzer collar tie drag torque on off-hand.

## Consequences
- Traumatic close-quarters structural post takedown and pin finish.
- Neutralizes aggressive adversaries using environmental vertical post geometry.
