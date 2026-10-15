# ADR 0261: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Ground Finish & Sternum Knee Drop

## Status
Accepted

## Context
When an inmate brawl adversary attempts to kick free from a grounded whizzer pin, dropping an explosive knee into their sternum while maintaining bilateral cervical collar control fractures the ribs and compresses vertebrae, terminating the fight conclusively.

## Decision
Implement physical whizzer sternum knee drop mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep overhook whizzer and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Knee Drop Compression**: Plunge both hands and body downward violently (`rVel.y < -1.55\text{ m/s} \land lVel.y < -1.30\text{ m/s} \land |lVel.x| > 0.85\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral pivot, crouch drop, and concussive sternum knee strike.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) sternum bone fracture shockwave on dominant hand, $240\text{ Hz}$ ($0.92$) collar tie tension on off-hand.

## Consequences
- Bone-crushing ground finisher concluding brutal pit fights.
- Smoothly integrates with whizzer dislocations, cervical spikes, and suplexes.
