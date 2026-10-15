# ADR 0215: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Shimming Washer Insertion

## Status
Accepted

## Context
Thermal expansion and mechanical wear create axial end-play in the solar collector truss hinge lugs, causing vibration flutter during spacecraft thruster burns. Riddick eliminates this backlash by inserting a slotted U-shaped stainless steel shim washer into the hinge gap.

## Decision
Implement physical slotted shim insertion in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position off-hand at the hinge gap collar ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$) and dominant hand holding the slotted shim ($dist < 0.22\text{ m}$, Right Grip $> 0.60$).
- **Lateral Seating Thrust**: Slide the shim washer laterally into the hinge clevis gap ($|rVel.x| > 0.85\text{ m/s}$) until it clicks into the retention detents.
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` for gap alignment, shim advancement, and detent seating.
- **Haptic Feedback**: $190\text{ Hz}$ ($0.65$) shim sliding friction followed by $280\text{ Hz}$ ($0.95$) mechanical gap detent seating click.

## Consequences
- Elevates precision mechanical spacewalk interactions.
- Pairs with circlips, drift pins, and grease cap servicing.
