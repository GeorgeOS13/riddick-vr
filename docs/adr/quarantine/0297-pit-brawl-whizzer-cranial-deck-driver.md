# ADR 0297: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Cranial Deck Driver

## Status
Accepted

## Context
When an inmate brawl adversary is pinned to the grating with their thoracic spine hyperextended, driving the dominant palm and body mass straight down into the base of their skull smashes their cranium into the steel deck grating to execute an instantaneous lethal submission.

## Decision
Implement physical whizzer thoracic spine hyperextension cranial deck driver mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Palm & Cranial Plunge**: Plunge dominant palm and body downward with extreme velocity (`rVel.y < -2.00\text{ m/s} \land lVel.y < -1.75\text{ m/s} \land |lVel.x| > 0.90\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral throw, crouch drop, and concussive cranial deck driver execution.
- **Haptic Feedback**: $390\text{ Hz}$ ($0.98$) cranial deck grating impact shockwave on dominant hand, $300\text{ Hz}$ ($0.95$) whizzer collar tie drag on off-hand.

## Consequences
- Pinnacle pit brawl ground execution finisher.
- Seamlessly completes the progression from whizzer overhooks to sternum drops and cranial impacts.
