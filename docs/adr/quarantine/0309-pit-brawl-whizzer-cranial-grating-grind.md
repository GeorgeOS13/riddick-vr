# ADR 0309: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Grating Grind

## Status
Accepted

## Context
When an inmate brawl opponent is pinned to the deck with their thoracic spine hyperextended, driving their cranium forward and downward into the sharp diamond-pattern steel floor grating while dragging them laterally grinds facial bones across the deck to compel an immediate lethal submission.

## Decision
Implement physical whizzer thoracic spine hyperextension cranial grating grind mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward-Forward Grating Drag Plunge**: Plunge dominant palm and body downward with extreme velocity (`rVel.y < -2.15\text{ m/s} \land lVel.y < -1.90\text{ m/s} \land |lVel.x| > 0.90\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral throw, crouch drop, and abrasive cranial deck grating grind execution.
- **Haptic Feedback**: $410\text{ Hz}$ ($0.98$) cranial diamond grating abrasion shockwave on dominant hand, $330\text{ Hz}$ ($0.95$) whizzer collar tie drag on off-hand.

## Consequences
- Devastating pit brawl ground execution finisher.
- Seamlessly pairs with cervical deck pins, sternum knee spikes, and cranial deck drivers.
