# ADR 0293: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Sternum Elbow Drop

## Status
Accepted

## Context
When an inmate brawl adversary is pinned to the grating with their thoracic spine hyperextended, elevating the dominant elbow and plunging it downward directly into their sternum delivers devastating blunt-force trauma that instantly shatters ribs and finishes the pit encounter.

## Decision
Implement physical whizzer thoracic spine hyperextension sternum elbow drop mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), maintain deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Elbow Plunge**: Plunge dominant elbow and body downward with extreme velocity (`rVel.y < -1.95\text{ m/s} \land lVel.y < -1.70\text{ m/s} \land |lVel.x| > 0.90\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral throw, crouch drop, and concussive sternum elbow drop finisher.
- **Haptic Feedback**: $380\text{ Hz}$ ($0.98$) sternum fracture shockwave on dominant hand, $290\text{ Hz}$ ($0.95$) whizzer collar tie drag on off-hand.

## Consequences
- Devastating close-quarters pit brawl execution finisher.
- Cascades seamlessly from whizzer ground pins, thoracic dislocations, and cervical hyperextensions.
