# ADR 0273: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Grating Fracture Slam

## Status
Accepted

## Context
When an inmate brawl adversary attempts a desperate hip bump reversal against a whizzer pin, violently torquing the dominant forearm down while hauling their cervical spine downward onto the raised iron deck grating compresses vertebrae and fractures their neck guard.

## Decision
Implement physical whizzer cervical compression grating fracture slam mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Forearm & Body Plunge**: Plunge dominant forearm and body downward with extreme downward velocity (`rVel.y < -1.70\text{ m/s} \land lVel.y < -1.45\text{ m/s} \land |lVel.x| > 0.85\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral throw, crouch drop, and concussive cervical grating fracture.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) cervical vertebra fracture shockwave on dominant hand, $250\text{ Hz}$ ($0.92$) whizzer collar tie drag on off-hand.

## Consequences
- Concussive pit brawl ground finisher neutralizing elite inmate fighters.
- Integrates cleanly with thoracic grating slams, sternum bone fracture spikes, and whizzer dislocations.
