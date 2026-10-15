# ADR 0196: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Primary Air Swirl Vane Lever

## Status
Accepted

## Context
Controlling vortex combustion inside the bio-waste incinerator requires adjusting the primary combustion air swirl vanes. Riddick grasps the heavy quadrant lever on the combustion casing with both hands and sweeps horizontally across the quadrant arc.

## Decision
Implement physical dual-hand swirl vane quadrant lever sweep in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position hands at the combustion air swirl casing ($dist \in [0.20, 0.40]\text{ m}, Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.15, 0.25]\text{ m}$) with both grips clenched ($> 0.55$).
- **Quadrant Sweep**: Sweep both hands horizontally across the quadrant arc ($|V_{x, left}| > 0.70\text{ m/s} \land |V_{x, right}| > 0.70\text{ m/s}$) to position the swirl vanes.
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` for quadrant lever actuation, air vane repositioning, and combustion vortex optimization.
- **Haptic Feedback**: Dual $150\text{ Hz}$ ($0.65$) quadrant ratchet friction, followed by $240\text{ Hz}$ ($0.90$) detent notch engagement click.

## Consequences
- Deepens facility puzzle logic and airflow balancing in the subterranean waste recycling sector.
- Works in tandem with flame dampers, soot blowers, and preheater bypass systems.
