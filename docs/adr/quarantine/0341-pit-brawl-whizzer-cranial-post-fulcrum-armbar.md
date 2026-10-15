# ADR 0341: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Armbar Fracture

## Status
Accepted

## Context
When pinning an opponent against a vertical structural post, trapping their overhooked limb across the sharp 90-degree steel post corner while hyperextending their elbow joint creates an unyielding lever that fractures the olecranon process and completely neutralizes their weapon-bearing arm.

## Decision
Implement physical whizzer cranial post fulcrum leverage armbar fracture mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, both grips $> 0.65$, right grip $> 0.65$).
- **Violent Fulcrum Armbar Crank Plunge**: Plunge downward and drive opposing lateral torque across post corner (`rVel.y < -2.55\text{ m/s} \land lVel.y < -2.30\text{ m/s} \land |lVel.x| > 1.15\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` + `Space` + `C` + `LMB` + `Space` + `Q` + `Tab` + `Space` + `LMB` for corner limb trap, fulcrum joint hyperextension, and deck impact fracture.
- **Haptic Feedback**: $490\text{ Hz}$ ($0.98$) joint fracture and structural post shockwave on dominant hand, $410\text{ Hz}$ ($0.95$) whizzer collar tie drag torque on off-hand.

## Consequences
- Traumatic close-quarters joint manipulation and disarm.
- Renders high-tier adversaries unable to mount counter-attacks or hold heavy weapons.
