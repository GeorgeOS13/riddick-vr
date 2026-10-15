# ADR 0305: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cervical Deck Pin Spike

## Status
Accepted

## Context
When an inmate brawl adversary is pinned to the grating with their thoracic spine hyperextended, driving the dominant palm and body mass diagonally into their cervical-thoracic junction delivers a catastrophic grinding pin that immobilizes vertebrae and finishes the brawl encounter instantly.

## Decision
Implement physical whizzer thoracic spine hyperextension cervical deck pin spike mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward-Diagonal Body Spike Plunge**: Plunge dominant palm and body downward with extreme velocity (`rVel.y < -2.10\text{ m/s} \land lVel.y < -1.85\text{ m/s} \land |lVel.x| > 0.90\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral throw, crouch drop, and concussive cervical deck pin spike finisher.
- **Haptic Feedback**: $400\text{ Hz}$ ($0.98$) cervical vertebra fracture shockwave on dominant hand, $320\text{ Hz}$ ($0.95$) whizzer collar tie drag on off-hand.

## Consequences
- Decisive pit brawl ground submission and immobilization finisher.
- Seamlessly pairs with sternum knee spikes, cranial deck drivers, and thoracic dislocations.
