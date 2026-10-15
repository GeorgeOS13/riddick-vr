# ADR 0281: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Hyperextension Mat Dislocation

## Status
Accepted

## Context
When an inmate brawl adversary attempts an explosive bridge reversal against a grounded whizzer pin, whipping their head and neck laterally while driving a dominant forearm downward into their mid-thoracic spine hyperextends vertebrae and dislocates their spinal alignment, terminating the brawl conclusively.

## Decision
Implement physical whizzer thoracic spine hyperextension mat dislocation mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Forearm & Body Plunge**: Plunge dominant forearm and body downward with extreme velocity (`rVel.y < -1.80\text{ m/s} \land lVel.y < -1.55\text{ m/s} \land |lVel.x| > 0.85\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral throw, crouch drop, and concussive spinal hyperextension dislocation.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) thoracic vertebra dislocation shockwave on dominant hand, $260\text{ Hz}$ ($0.92$) whizzer collar tie drag on off-hand.

## Consequences
- Conclusive pit brawl ground finisher neutralizing elite inmate fighters.
- Seamlessly integrates with cervical hyperextension finishers, grating fracture slams, and whizzer dislocations.
