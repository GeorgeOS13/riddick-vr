# ADR 0264: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Amplifier Potentiometer

## Status
Accepted

## Context
Thermal expansion and pyrolytic haze inside the bio-waste incinerator blower shroud cause optical signal degradation. Trimming a multi-turn potentiometer on the optical tachometer signal amplifier module tunes pickup sensitivity and comparator trigger thresholds, eliminating jitter during high-RPM dynamic rotor balancing.

## Decision
Implement physical tachometer signal amplifier potentiometer adjustment mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the amplifier casing (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand aligns an insulated trimmer screwdriver at the potentiometer dial (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotational Trimmer Torque**: Apply clockwise rotational wrist torque with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to adjust gain, calibrate comparator thresholds, verify TTL square wave, and lock amplifier telemetry.
- **Haptic Feedback**: $200\text{ Hz}$ ($0.55$) potentiometer detent wiper micro-clicks on the dominant controller and $330\text{ Hz}$ ($0.95$) signal gain lock chime on the off-hand controller.

## Consequences
- Ensures clean square-wave TTL optical telemetry during draft fan spool-up.
- Seamlessly pairs with cable shield grounding screws, strain relief clamps, and sensor gimbal alignment.
