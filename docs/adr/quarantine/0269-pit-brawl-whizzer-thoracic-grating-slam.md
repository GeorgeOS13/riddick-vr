# ADR 0269: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Sternum Bone Fracture Thoracic Grating Slam

## Status
Accepted

## Context
When an inmate brawl adversary attempts an escape whizzer roll on the ground, countering by violently driving the dominant elbow down into their sternum while pulling their collar into the steel deck grating fractures ribs and terminates the brawl instantly.

## Decision
Implement physical whizzer sternum bone fracture thoracic grating slam mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Elbow & Body Plunge**: Plunge dominant forearm and body downward with maximum downward velocity (`rVel.y < -1.65\text{ m/s} \land lVel.y < -1.40\text{ m/s} \land |lVel.x| > 0.85\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral pivot, crouch drop, and concussive thoracic grating slam.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) thoracic cage fracture shockwave on dominant hand, $250\text{ Hz}$ ($0.92$) whizzer collar tie drag on off-hand.

## Consequences
- Concussive pit brawl ground finisher neutralizing elite inmate fighters.
- Integrates cleanly with sternum bone fracture spikes, whizzer dislocations, and cervical mat spikes.
