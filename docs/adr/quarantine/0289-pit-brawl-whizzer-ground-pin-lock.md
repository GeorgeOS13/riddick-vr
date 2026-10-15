# ADR 0289: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Ground Pin Lock

## Status
Accepted

## Context
When an inmate brawl adversary attempts an explosive upward hip bridge against a whizzer pin, trapping their wrist into an americana keylock while violently driving the dominant forearm down into their thoracic spine compresses their ribs against the deck grating and immobilizes them conclusively.

## Decision
Implement physical whizzer thoracic spine hyperextension ground pin lock mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Forearm & Body Plunge**: Plunge dominant forearm and body downward with extreme velocity (`rVel.y < -1.90\text{ m/s} \land lVel.y < -1.65\text{ m/s} \land |lVel.x| > 0.85\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral throw, crouch drop, and concussive keylock ground pin.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) shoulder joint keylock fracture shockwave on dominant hand, $270\text{ Hz}$ ($0.92$) whizzer collar tie drag on off-hand.

## Consequences
- Conclusive pit brawl ground submission and immobilization finisher.
- Seamlessly integrates with thoracic mat slam finishers, thoracic dislocations, and cervical hyperextension finishers.
