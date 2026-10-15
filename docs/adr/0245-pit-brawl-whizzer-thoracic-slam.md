# ADR 0245: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Grating Slam

## Status
Accepted

## Context
When an adversary braces against neck-cracking throws by dropping their center of gravity, shifting the whizzer fulcrum into the thoracic ribcage allows Riddick to elevate and whip them laterally, slamming their thoracic spine and ribs across the steel pit deck grating.

## Decision
Implement physical whizzer thoracic grating slam mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Thoracic Whizzer Grip**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep overhook and thoracic underhook (`leftLocal.y` between $-0.20\text{ m}$ and $0.15\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Rotational Deck Slam**: Whip dominant elbow down violently (`rVel.y < -1.30\text{ m/s}`) while hauling the off-hand laterally and upward (`|lVel.x| > 0.90\text{ m/s} \land lVel.y > 0.60\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral pivot, crouch drop, and thoracic grating impact.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) ribcage crush and deck grating impact shockwave on dominant hand, $220\text{ Hz}$ ($0.90$) thoracic squeeze lift strain on off-hand.

## Consequences
- High-impact throw breaking sprawling opponents in brutal inmate pit brawls.
- Pairs with cervical spikes, whizzer dislocations, and hip tosses.
