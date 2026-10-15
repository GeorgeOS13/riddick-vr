# ADR 0321: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Post Double-Strike Execution

## Status
Accepted

## Context
In close-quarters pit brawls, after driving an opponent sideways into an arena I-beam post, rebounding their dazed skull with an immediate secondary lateral whip smash completes a brutal cranial double-strike execution, ensuring a traumatic structural knockout.

## Decision
Implement physical whizzer thoracic spine hyperextension cranial post double-strike execution mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward-Lateral Whip Double-Strike Plunge**: Plunge downward and whip laterally with extreme velocity (`rVel.y < -2.30\text{ m/s} \land lVel.y < -2.05\text{ m/s} \land |lVel.x| > 0.95\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` + `Space` + `LMB` for initial structural beam cranial slam followed immediately by a rebounding secondary concussive post impact.
- **Haptic Feedback**: $440\text{ Hz}$ ($0.98$) dual cranial structural impact shockwave on dominant hand, $360\text{ Hz}$ ($0.95$) whizzer collar tie drag on off-hand.

## Consequences
- Traumatic, brutal environmental execution.
- Advances pit fighting close-quarter lethal dominance against heavyweight opponents.
