# ADR 0252: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Phase Angle Optical Tachometer Sensor Alignment

## Status
Accepted

## Context
Accurately registering the reflective tape burnished to the draft fan shaft requires fine optical sensor alignment. Adjusting the dual-axis optical tachometer bracket until the photoreceiver focal beam aligns with the reflective strip confirms zero-phase balance tracking before full furnace draft ramp-up.

## Decision
Implement physical optical tachometer sensor alignment mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the optical bracket clamp (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand adjusts the sensor barrel gimbal (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotational Gimbal Alignment**: Apply clockwise rotational adjustment with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to lock the optical sensor bracket, register beam alignment, and initiate continuous draft monitoring.
- **Haptic Feedback**: $190\text{ Hz}$ ($0.55$) optical receiver pulse micro-clicks on the right controller and $320\text{ Hz}$ ($0.95$) optical alignment lock chime on the left controller.

## Consequences
- Guarantees permanent optical feedback for subterranean hydroponics draft fan balancing.
- Pairs with reflective tape burnishing, stroboscope synchronization, and tang crimping.
