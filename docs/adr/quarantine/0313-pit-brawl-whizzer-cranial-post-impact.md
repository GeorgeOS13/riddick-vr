# ADR 0313: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Post-Impactor Slam

## Status
Accepted

## Context
When an inmate brawl opponent is pinned to the deck with their thoracic spine hyperextended, lifting their head slightly and slamming their cranium sideways with violent lateral momentum into an arena structural I-beam post executes a concussive environmental finish that instantly ends the brawl.

## Decision
Implement physical whizzer thoracic spine hyperextension cranial post-impactor slam mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward-Lateral Post Impact Plunge**: Plunge dominant palm and body downward and laterally with extreme velocity (`rVel.y < -2.20\text{ m/s} \land lVel.y < -1.95\text{ m/s} \land |lVel.x| > 0.95\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral throw, crouch drop, and concussive cranial structural post impact slam execution.
- **Haptic Feedback**: $420\text{ Hz}$ ($0.98$) cranial steel post collision shockwave on dominant hand, $340\text{ Hz}$ ($0.95$) whizzer collar tie drag on off-hand.

## Consequences
- Traumatic environmental pit brawl finisher.
- Seamlessly pairs with cranial grating grinds, cervical deck pins, and sternum knee spikes.
