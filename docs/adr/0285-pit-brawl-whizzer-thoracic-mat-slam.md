# ADR 0285: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Mat Slam Finisher

## Status
Accepted

## Context
When an inmate brawl opponent attempts a desperate scramble from a grounded whizzer lock, violently wrenching their collar tie downward and driving the dominant forearm through their upper thoracic vertebrae into the steel floor grating fractures ribs and terminates the brawl conclusively.

## Decision
Implement physical whizzer thoracic spine hyperextension mat slam finisher mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Forearm & Body Plunge**: Plunge dominant forearm and body downward with extreme velocity (`rVel.y < -1.85\text{ m/s} \land lVel.y < -1.60\text{ m/s} \land |lVel.x| > 0.85\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral throw, crouch drop, and concussive thoracic grating slam finisher.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) thoracic cage fracture shockwave on dominant hand, $270\text{ Hz}$ ($0.92$) whizzer collar tie drag on off-hand.

## Consequences
- Conclusive pit brawl ground finisher neutralizing elite inmate fighters.
- Seamlessly integrates with thoracic dislocations, cervical hyperextension finishers, and grating slams.
