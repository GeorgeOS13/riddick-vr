# ADR 0224: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Primary Air Plenum Cleanout Manhole Cover Dog Clamp

## Status
Accepted

## Context
Combustion fly ash and clinker fallout accumulate in the bottom plenum chamber beneath the primary incineration hearth. To inspect and purge this chamber, Riddick must release or secure the forged dog clamps dogging the oval access manhole cover.

## Decision
Implement physical dog-clamp levering in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position both hands at the plenum access collar ($dist \in [0.20, 0.40]\text{ m}, Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.35, 0.05]\text{ m}$, both grips $> 0.55$).
- **Synchronized Lever Pull**: Execute a synchronized downward pull ($V_{y, left} < -0.75\text{ m/s} \land V_{y, right} < -0.75\text{ m/s}$) to seat or lever open the forged dog clamps.
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` for dog-clamp rotation, asbestos gasket compression, and plenum cover lock.
- **Haptic Feedback**: $130\text{ Hz}$ ($0.65$) cast-iron clamp hinge swing followed by dual $270\text{ Hz}$ ($0.95$) forged dog-clamp seating thuds.

## Consequences
- Deepens industrial waste-sector environmental interaction mechanics.
- Coordinates with ash rakes, combustion swirl vanes, and damper linkages.
