# ADR 0187: Dark Athena EVA Solar Collector Articulation Hinge Wedge Removal Slide-Hammer

## Status
Accepted

## Context
During maintenance or demolition of damaged solar collector assemblies on the Dark Athena exterior hull, securing locking wedges can bind inside their tapered hinge seats due to vacuum micro-welding. Riddick uses an improvised EVA slide hammer, grasping the weighted slide with both hands, throwing the slide weight rearward, and impacting the end stop to draw out the seized lock wedge.

## Decision
Implement physical two-handed slide-hammer wedge extraction detection in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position both hands at the articulation hinge bracket ($dist < 0.26\text{ m}, Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.42]\text{ m}$) with both grips clenched ($> 0.60$).
- **Inertial Slide Haul**: Execute a violent synchronized rearward slide stroke ($V_{z, right} > 0.95\text{ m/s} \land V_{z, left} > 0.95\text{ m/s}$) to pop the wedge from its collar.
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` for mechanical wedge extraction and hinge unlocking.
- **Haptic Feedback**: Dual $180\text{ Hz}$ ($0.65$) slide travel friction followed by $290\text{ Hz}$ ($0.98$) inertia stop collision thud.

## Consequences
- Expands zero-gravity tool handling mechanics for deep space EVA salvage and structural manipulation.
- Complements mallet seating, cotter pin extraction, and lock ring spanner interactions.
