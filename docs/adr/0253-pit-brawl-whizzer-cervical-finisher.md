# ADR 0253: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Ground Finisher

## Status
Accepted

## Context
Following a successful whizzer counter-throw, grounding the opponent while retaining the overhook collar control allows Riddick to drop full body weight directly through their cervical spine into the concrete, concluding the pit brawl with a decisive concussive finisher.

## Decision
Implement physical whizzer cervical compression ground finisher mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep overhook and collar tie (`leftLocal.y` between $-0.25\text{ m}$ and $0.10\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Ground Compression Drop**: Plunge dominant forearm down with high velocity (`rVel.y < -1.45\text{ m/s}`) while hauling the off-hand straight downward into the mat (`lVel.y < -1.15\text{ m/s} \land |lVel.x| > 0.80\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral pivot, crouch drop, and downward cervical finisher.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) bone fracture impact shockwave on dominant hand, $220\text{ Hz}$ ($0.90$) collar tie downward drag on off-hand.

## Consequences
- Devastating terminal grappling sequence against formidable pit brawlers.
- Chains smoothly with hyperextension mat spikes, thoracic slams, and whizzer dislocations.
