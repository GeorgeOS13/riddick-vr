# ADR 0272: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Cable Silicone Strain Relief Boot

## Status
Accepted

## Context
Turbulent thermal convection pulses and high-frequency vibration inside the bio-waste incinerator draft blower room fatigue the shielded telemetry harness at the gland entry. Sliding a high-temperature silicone strain relief boot over the cable shield and gland fitting distributes bending stress and prevents conductor shear during prolonged incinerator operation.

## Decision
Implement physical tachometer signal cable silicone strain relief boot mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the cable gland adapter (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand aligns the silicone boot applicator (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Linear Sliding Insertion Stroke**: Drive the right hand forward along the cable axis (`rVel.z < -0.80\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to slide the strain relief boot, seat the elastomeric collar over the gland nut, engage vibration damping, and confirm harness stability.
- **Haptic Feedback**: $270\text{ Hz}$ ($0.95$) elastomeric boot detent snap on the dominant controller and $140\text{ Hz}$ ($0.55$) gland adapter friction on the off-hand controller.

## Consequences
- Guarantees prolonged fatigue resistance for optical tachometer wiring under extreme blower vibration.
- Integrates cleanly with magnetic shield wraps, potentiometer adjustments, and grounding screws.
