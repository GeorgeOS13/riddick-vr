# ADR 0191: Dark Athena EVA Solar Collector Heat Radiator Coolant Loop Bleed Petcock Valve

## Status
Accepted

## Context
On the exterior radiator wings of the Dark Athena, liquid-metal sodium coolant loops can develop vapor locks after micrometeoroid impacts. Riddick must locate the recessed bleed petcock valve on the manifold, clamp onto the square valve stem with pliers or gloved fingers, and twist 90 degrees counter-clockwise to vent overheated vapor into the vacuum.

## Decision
Implement physical radiator coolant bleed petcock valve twist detection in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Reach to radiator coolant manifold socket ($Z \in [-0.42, -0.15]\text{ m}, Y \in [-0.20, 0.20]\text{ m}$) with dominant hand in pinch grip ($Grip \in [0.45, 0.85] \land Trigger < 0.40$).
- **Rotational Vent Twist**: Apply a sharp rotational wrist twist ($|\Delta\text{roll}| > 1.2\text{ rad} \lor |V_x| > 0.65\text{ m/s}$) to pop open the petcock bleed valve.
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` for valve unseating, bypass purge, and coolant pressure normalization.
- **Haptic Feedback**: $170\text{ Hz}$ ($0.60$) valve unseating click, followed by $90\text{ Hz}$ ($0.85$) cryogenic vapor venting surge.

## Consequences
- Deepens zero-gravity thermal regulation puzzles during exterior Dark Athena spacewalk sequences.
- Pairs smoothly with radiator shutter louver and radiator fin unfolding interactions.
