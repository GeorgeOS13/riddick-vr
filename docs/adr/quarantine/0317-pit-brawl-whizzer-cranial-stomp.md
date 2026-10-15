# ADR 0317: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Deck Grating Stomp Finisher

## Status
Accepted

## Context
When an inmate brawl opponent is pinned to the deck with thoracic hyperextension, stepping up with the lead foot and driving a heavy heel stomp straight down into their cranium against the steel diamond deck grating delivers an immediate catastrophic skull fracture knockout.

## Decision
Implement physical whizzer thoracic spine hyperextension cranial deck grating stomp finisher mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward-Lateral Plunge with Heel Stomp Momentum**: Plunge dominant fist and body downward with extreme velocity (`rVel.y < -2.25\text{ m/s} \land lVel.y < -2.00\text{ m/s} \land |lVel.x| > 0.95\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` + `Space` for forward compression, lateral throw, crouch drop, concussive cranial deck grating heel stomp execution.
- **Haptic Feedback**: $430\text{ Hz}$ ($0.98$) cranial deck grating fracture shockwave on dominant hand, $350\text{ Hz}$ ($0.95$) whizzer collar tie drag on off-hand.

## Consequences
- Traumatic, visceral floor execution.
- Concludes the Whizzer counter-throw series with a definitive environmental floor strike.
