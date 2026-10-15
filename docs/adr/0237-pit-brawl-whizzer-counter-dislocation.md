# ADR 0237: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Shoulder Dislocation

## Status
Accepted

## Context
When an opponent attempts to defend against a standing collar tie by reaching under the armpit, Riddick hooks an overhook whizzer lever, rotates laterally, and drives the opponent's trapped arm downward to dislocate the shoulder joint while spiking them into the steel floor.

## Decision
Implement physical whizzer counter-throw mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook Positioning**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), secure overhook arm trap (`leftLocal.y` between $-0.15\text{ m}$ and $0.15\text{ m}$, left grip $> 0.65$, right grip $> 0.65$).
- **Rotational Downward Whip**: Drive dominant hand down violently (`rVel.y < -1.25\text{ m/s}`) while whipping the off-hand laterally (`|lVel.x| > 0.95\text{ m/s}`).
- **Dynamic Direction**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` for forward drive, lateral pivot, crouch leverage, mat elevation, and joint dislocation strike.
- **Haptic Feedback**: $350\text{ Hz}$ ($0.98$) shoulder socket capsule tear and bone crack on dominant hand, $210\text{ Hz}$ ($0.85$) overhook fulcrum strain on off-hand.

## Consequences
- Devastating clinch defense counter-throw against aggressive pit fighters.
- Chains naturally with lateral suplexes, Thai plum neck snaps, and hip tosses.
