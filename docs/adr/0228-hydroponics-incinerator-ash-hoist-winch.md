# ADR 0228: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Primary Air Plenum Ash Hoist Bucket Winch Crank

## Status
Accepted

## Context
With the plenum cleanout dog clamps released (ADR 0224), heavy vitreous ash and slag buildup must be evacuated from the lower plenum pit using a manual worm-drive hoist bucket winch mounted to the casing bulkhead. Riddick cranks the winch handle to hoist the slag bucket to floor level.

## Decision
Implement physical ash hoist bucket winch cranking in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position dominant hand at the wall winch crank hub ($Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.20, 0.25]\text{ m}$, $Grip > 0.55$).
- **Rotational Cranking**: Execute 3 full circular revolutions in the frontal plane ($|\vec{V}_{xy}| > 0.48\text{ m/s}$).
- **Synthesized Controls**: Emits `E` + `Q` + `Space` + `Tab` for winch clutch engagement, pawl ratcheting, and bucket carriage lift.
- **Haptic Feedback**: $140\text{ Hz}$ ($0.55$) cable drum winding rumble and $220\text{ Hz}$ ($0.95$) mechanical hoist dog ratchet thud.

## Consequences
- Expands vertical multi-tiered industrial puzzle mechanics in the incinerator complex.
- Works in tandem with dog clamps, ash rakes, and primary swirl vanes.
