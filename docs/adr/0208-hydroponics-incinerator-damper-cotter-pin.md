# ADR 0208: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Combustion Air Damper Linkage Cotter Pin Extraction

## Status
Accepted

## Context
Corrosion from sulfuric flue gases seizes the primary combustion air damper linkages. Riddick frees the linkage by grasping the clevis pin retaining cotter pin and pulling it backward to free the stuck damper arm.

## Decision
Implement physical two-handed damper cotter pin extraction in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position off-hand on the damper clevis bracket ($Z \in [-0.40, -0.16]\text{ m}, Y \in [-0.15, 0.25]\text{ m}$, Left Grip $> 0.55$) and dominant hand on the pin eyelet ($dist < 0.22\text{ m}$, Right Grip $> 0.60$).
- **Linear Extraction Haul**: Deliver a high-velocity rearward extraction haul ($rVel.z > 0.90\text{ m/s}$) to yank the cotter pin free from the clevis bore.
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` for mechanical clevis stabilization, pin withdrawal, and linkage release.
- **Haptic Feedback**: $210\text{ Hz}$ ($0.65$) corroded clevis drag followed by $290\text{ Hz}$ ($0.98$) cotter pin release snap.

## Consequences
- Adds mechanical authenticity to the subterranean bio-waste incinerator restoration sequence.
- Complements air swirl vane levers, damper handwheels, and soot blowers.
