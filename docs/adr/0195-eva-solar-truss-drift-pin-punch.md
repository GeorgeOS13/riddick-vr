# ADR 0195: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Alignment Drift Pin Punch

## Status
Accepted

## Context
Thermal expansion during Dark Athena orbital day-night cycles causes misalignment between the collector truss hinge lugs. Riddick aligns the hinge lugs by driving a tapered alignment drift pin through the outer collar using a two-handed punch-and-hammer technique.

## Decision
Implement physical two-handed drift pin alignment punch in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position off-hand on the alignment collar ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$) and dominant hand above holding mallet/punch ($dist < 0.24\text{ m}$, Right Grip $> 0.60$).
- **Dynamic Punch Strike**: Deliver a sharp dominant downward hammer strike ($rVel.y < -1.15\text{ m/s}$) to drive the tapered drift pin into the misaligned hinge lugs.
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Space` for alignment tool engagement and mechanical lug centering.
- **Haptic Feedback**: $260\text{ Hz}$ ($0.98$) alloy pin punch shockwave, followed by $140\text{ Hz}$ ($0.65$) mechanical lug alignment detent pop.

## Consequences
- Enhances realistic exterior spacewalk mechanical repair and sabotage gameplay.
- Integrates with cotter pin, mallet strike, and slide-hammer EVA tool suites.
