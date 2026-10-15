# ADR 0256: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Cable Strain Relief Clamp Screw

## Status
Accepted

## Context
High draft velocity and thermal turbulence within the fan housing can vibrate the optical tachometer signal lead, inducing false phase jitter. Securing the signal cable inside a strain relief clamp on the housing pedestal isolates the harness from airflow vibrations.

## Decision
Implement physical tachometer cable strain relief clamp screw mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the cable harness inside the strain relief saddle (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand aligns a slotted screwdriver at the clamp screw (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotational Torquing Action**: Apply clockwise rotational wrist torque with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to torque the strain relief clamp, eliminate harness vibration, and lock optical telemetry.
- **Haptic Feedback**: $280\text{ Hz}$ ($0.95$) mechanical saddle clamp torque stop on the right controller and $140\text{ Hz}$ ($0.55$) shielded cable compression resistance on the left controller.

## Consequences
- Stabilizes optical telemetry during full furnace blast induction.
- Seamlessly pairs with sensor alignment, tape burnishing, and stroboscope synchronization.
