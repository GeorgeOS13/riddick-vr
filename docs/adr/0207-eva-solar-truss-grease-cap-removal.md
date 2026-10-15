# ADR 0207: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Grease Nipple Cap Removal

## Status
Accepted

## Context
Before lubricating seized collector truss articulation hinges in deep space vacuum, Riddick must remove the protective threaded sealing cap from the grease nipple fitting on the truss collar to expose the zerk ball check.

## Decision
Implement physical grease nipple cap unthreading in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position dominant hand at the truss hinge grease fitting ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.08, 0.38]\text{ m}$) with a pinch grip ($Grip \in [0.45, 0.85] \land Trigger < 0.40$).
- **Rotational Twist**: Apply a sharp counter-clockwise rotational wrist twist ($|V_x| > 0.65\text{ m/s}$) to unseat and spin off the cap.
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `'Q'` for socket alignment, seal break, and dust cap unthreading.
- **Haptic Feedback**: $170\text{ Hz}$ ($0.55$) thread unseating friction followed by $270\text{ Hz}$ ($0.90$) detent click.

## Consequences
- Deepens multi-stage exterior ship maintenance puzzles.
- Pairs directly with grease gun nozzle injection and articulation lock ring mechanics.
