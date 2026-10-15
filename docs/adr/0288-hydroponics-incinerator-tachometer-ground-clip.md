# ADR 0288: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Probe Ground Clip Attachment

## Status
Accepted

## Context
Calibrating pulse symmetry and jitter on the bio-waste incinerator induced draft fan optical tachometer requires an ultra-low-inductance oscilloscope ground reference. Clamping a spring-loaded gold-plated alligator clip to the amplifier chassis grounding post provides a clean return path for nanosecond-speed TTL edge measurements.

## Decision
Implement physical tachometer oscilloscope probe ground clip attachment mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand stabilizes the amplifier ground post (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand aligns the spring-loaded ground clip (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Linear Clip Attachment Stroke**: Drive the right hand forward onto the ground post (`rVel.z < -0.75\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to open the clip jaws, clamp onto the grounding stud, verify electrical ground, and initialize oscilloscope channel calibration.
- **Haptic Feedback**: $280\text{ Hz}$ ($0.95$) spring alligator clip bite snap on the dominant controller and $140\text{ Hz}$ ($0.55$) ground post contact on the off-hand controller.

## Consequences
- Guarantees high-fidelity pulse shape and jitter verification on draft fan optical balance telemetry.
- Seamlessly pairs with BNC cable P-clamps, BNC twist-locks, and heat-shrink curing.
