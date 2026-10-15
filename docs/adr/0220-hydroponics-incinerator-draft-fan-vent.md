# ADR 0220: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Bearing Housing Oil Level Sight-Glass Inspection & Vent

## Status
Accepted

## Context
Induced draft fans maintain continuous negative furnace pressure across the incinerator boiler tubes. Bearing seizure caused by contaminated lubricant or air locks triggers draft fan trips and chamber backdraft. Riddick inspects the bearing sight-glass and cracks open the knurled pressure vent plug on the bearing pedestal.

## Decision
Implement physical bearing vent unseating in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position dominant hand at the fan pedestal bearing housing ($Z \in [-0.42, -0.16]\text{ m}, Y \in [0.05, 0.35]\text{ m}$) with pinch grip ($Grip \in [0.45, 0.85] \land Trigger < 0.40$).
- **Vent Twist**: Execute a sharp counter-clockwise rotational wrist twist ($|V_x| > 0.65\text{ m/s}$) to unseat the knurled vent plug.
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` for sight-glass clearance, vent unthreading, and draft lubrication equilibration.
- **Haptic Feedback**: $150\text{ Hz}$ ($0.55$) knurled brass thread friction followed by $95\text{ Hz}$ ($0.85$) pneumatic oil chamber vent surge.

## Consequences
- Expands industrial facility maintenance puzzles in the lower waste sector.
- Works in harmony with secondary damper wheels, thermocouple wells, and rapping rods.
