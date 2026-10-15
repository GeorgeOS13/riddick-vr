# ADR 0219: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Retaining Cap Screw Driver Torque

## Status
Accepted

## Context
Following shimming washer (ADR 0215) and circlip insertion (ADR 0211), the solar collector truss hinge lug must be locked axially with an M12 countersunk hex retaining cap screw to prevent vibration creep during main engine burn cycles. Riddick uses a hex driver bit to torque the retaining screw into the countersunk boss.

## Decision
Implement physical retaining screw torqueing in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position off-hand at the hinge clevis collar ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$) and dominant hand with hex driver at the countersunk boss ($dist < 0.22\text{ m}$, Right Grip $> 0.55 \land Right Trigger < 0.40$).
- **Rotational Driver Torque**: Execute a sharp clockwise rotational wrist twist ($|V_x| > 0.65\text{ m/s}$) to torque the screw home.
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` for screw thread engagement, lead thread uptake, and final hex socket torque lock.
- **Haptic Feedback**: $170\text{ Hz}$ ($0.60$) helical thread tightening friction followed by $290\text{ Hz}$ ($0.95$) mechanical metal-on-metal torque stop thud.

## Consequences
- Completes the precision multi-step EVA collector hinge maintenance sequence.
- Integrates seamlessly with shim washers, circlips, and drift pins.
