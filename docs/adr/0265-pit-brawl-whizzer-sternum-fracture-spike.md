# ADR 0265: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Sternum Bone Fracture Mat Spike

## Status
Accepted

## Context
When a grounded inmate adversary attempts an escape bridge from a whizzer pin, executing an explosive hip turn and driving the dominant forearm down while slamming a knee directly across their ribcage fractures the sternum, compressing the thoracic cavity and terminating the fight conclusively.

## Decision
Implement physical whizzer sternum bone fracture mat spike mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Forearm & Body Plunge**: Plunge dominant forearm and body downward with maximum downward velocity (`rVel.y < -1.60\text{ m/s} \land lVel.y < -1.35\text{ m/s} \land |lVel.x| > 0.85\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral pivot, crouch drop, and concussive sternum impact.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) sternum bone fracture shockwave on dominant hand, $250\text{ Hz}$ ($0.92$) collar tie tension on off-hand.

## Consequences
- Conclusive pit brawl ground finisher neutralizing elite inmate fighters.
- Integrates cleanly with sternum knee drops, whizzer dislocations, and cervical mat spikes.
