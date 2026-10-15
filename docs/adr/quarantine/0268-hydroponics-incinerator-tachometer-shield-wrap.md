# ADR 0268: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Cable Magnetic Shield Wrap

## Status
Accepted

## Context
High-voltage electrostatic precipitators adjacent to the bio-waste incinerator induced draft fan generate electromagnetic eddy currents that distort optical tachometer telemetry. Wrapping a flexible mu-metal magnetic shielding foil around the signal cable adjacent to the motor pedestal isolates the low-amplitude pulses and guarantees noise-free balance telemetry.

## Decision
Implement physical tachometer signal cable magnetic shield wrap mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand guides the cable harness near the motor pedestal (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand wraps the mu-metal foil sleeve (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotational Wrapping Stroke**: Apply tangential wrapping wrist motion with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` to wrap the mu-metal sleeve, bond the magnetic barrier, ground eddy currents, and certify clean telemetry.
- **Haptic Feedback**: $260\text{ Hz}$ ($0.95$) metallic shielding wrap click on the dominant controller and $150\text{ Hz}$ ($0.55$) silicone cable strain on the off-hand controller.

## Consequences
- Eliminates EMI hum and eddy-current distortion from incinerator draft fan balance telemetry.
- Pairs with potentiometer adjustments, grounding screws, and cable strain relief clamps.
