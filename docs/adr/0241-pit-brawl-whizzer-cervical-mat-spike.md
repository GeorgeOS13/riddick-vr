# ADR 0241: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Cervical Compression Mat Spike

## Status
Accepted

## Context
In deep pit brawls against heavy armored inmates, throwing an opponent solely by the shoulder can fail if they sprawl. Wrapping an overhook whizzer while concurrently hooking the cervical spine with the off-hand allows Riddick to spike their head and neck directly into the metal deck, fracturing vertebrae.

## Decision
Implement physical whizzer cervical compression mat spike mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Whizzer Grip**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock overhook whizzer and neck tie (`leftLocal.y` between $-0.10\text{ m}$ and $0.20\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Violent Downward Double Whip**: Drive dominant elbow downward with high velocity (`rVel.y < -1.35\text{ m/s}`) combined with strong inward/downward pull on the neck (`lVel.y < -1.05\text{ m/s} \land |lVel.x| > 0.85\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward compression, lateral pivot, crouch drop, and spinal compression slam.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) cervical vertebra fracture shockwave on dominant hand, $220\text{ Hz}$ ($0.90$) collar tie neck drag on off-hand.

## Consequences
- High-damage grapple finish from contested overhook exchanges.
- Smoothly complements whizzer dislocations, lateral suplexes, and Thai plum knee strikes.
