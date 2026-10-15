# ADR 0301: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Sternum Knee Spike Finisher

## Status
Accepted

## Context
When an inmate brawl adversary is pinned to the grating with their thoracic spine hyperextended, elevating the lead knee and driving it downward with full body momentum directly into their sternum fractures ribs and collapses the thoracic cavity to finish the pit match instantly.

## Decision
Implement physical whizzer thoracic spine hyperextension sternum knee spike finisher mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Knee Spike Plunge**: Plunge body and lead knee downward with extreme velocity (`rVel.y < -2.05\text{ m/s} \land lVel.y < -1.80\text{ m/s} \land |lVel.x| > 0.90\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral throw, crouch drop, and concussive sternum knee spike finisher.
- **Haptic Feedback**: $400\text{ Hz}$ ($0.98$) sternum fracture shockwave on dominant hand, $310\text{ Hz}$ ($0.95$) whizzer collar tie drag on off-hand.

## Consequences
- Devastating close-quarters pit brawl execution finisher.
- Cascades seamlessly from whizzer overhooks, thoracic dislocations, and cranial deck drivers.
