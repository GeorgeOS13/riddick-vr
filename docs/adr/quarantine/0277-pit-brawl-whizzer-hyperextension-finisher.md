# ADR 0277: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Grating Hyperextension Finisher

## Status
Accepted

## Context
When an inmate brawl adversary attempts a cervical posture escape against a whizzer pin, whipping their chin upward while driving the whizzer forearm downward onto the floor grating hyperextends the cervical spine and finishes the fight conclusively.

## Decision
Implement physical whizzer cervical compression grating hyperextension finisher mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Forearm & Body Plunge**: Plunge dominant forearm and body downward with extreme velocity (`rVel.y < -1.75\text{ m/s} \land lVel.y < -1.50\text{ m/s} \land |lVel.x| > 0.85\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral throw, crouch drop, and concussive hyperextension finisher.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) cervical hyperextension fracture shockwave on dominant hand, $260\text{ Hz}$ ($0.92$) whizzer collar tie drag on off-hand.

## Consequences
- Conclusive pit brawl ground finisher neutralizing elite inmate fighters.
- Integrates cleanly with grating fracture slams, sternum bone fracture spikes, and whizzer dislocations.
