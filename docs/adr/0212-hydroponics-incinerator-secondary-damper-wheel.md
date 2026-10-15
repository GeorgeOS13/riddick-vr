# ADR 0212: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Secondary Air Damper Handwheel Spin

## Status
Accepted

## Context
Secondary air preheating stabilizes bio-waste combustion temperatures and prevents toxic dioxin formation. Riddick reaches up to the overhead combustion flue casing to crank the secondary air damper handwheel through multiple revolutions to open the internal secondary draft vanes.

## Decision
Implement physical secondary damper handwheel cranking in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position dominant hand at overhead handwheel hub ($Z \in [-0.42, -0.16]\text{ m}, Y \in [0.10, 0.42]\text{ m}$) with grip clenched ($> 0.55$).
- **Rotational Spindle Revolutions**: Execute 3 full circular revolutions in the frontal plane ($|\vec{V}_{xy}| > 0.48\text{ m/s}$) to open damper vanes.
- **Synthesized Controls**: Emits `E` + `Q` + `Space` + `Tab` for spindle rotation, secondary draft vane opening, and flue gas optimization.
- **Haptic Feedback**: $140\text{ Hz}$ ($0.55$) spindle rotation friction and $220\text{ Hz}$ ($0.95$) mechanical end-stop thud.

## Consequences
- Solidifies industrial environmental control gameplay in the bio-waste reclamation zone.
- Works in harmony with primary air swirl vanes, precipitator rapping rods, and ash trough agitators.
