# ADR 0300: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Channel Input Coupling Toggle (AC/DC/GND)

## Status
Accepted

## Context
During dynamic balance telemetry acquisition on the induced draft fan rotor, DC offset drift in the optical tachometer pre-amplifier can shift pulse peaks off-screen. Toggling the oscilloscope channel input coupling slide switch from DC to AC coupling inserts a blocking capacitor, centering the pulse train symmetrically on the graticule zero volt line.

## Decision
Implement physical tachometer diagnostic oscilloscope channel input coupling toggle mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the oscilloscope lower chassis bezel (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand positions index finger on coupling slide switch (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Vertical Slide Stroke**: Slide the right hand vertically across switch detents (`|rVel.y| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Return` to toggle coupling slide switch, filter DC baseline drift, center tachometer pulse waveform, and lock dynamic balance phase reference.
- **Haptic Feedback**: $300\text{ Hz}$ ($0.95$) 3-position slide switch detent click on dominant controller and $130\text{ Hz}$ ($0.50$) chassis grounding vibration on off-hand controller.

## Consequences
- Guarantees baseline symmetry and eliminates DC bias on optical tachometer signals.
- Precedes final rotor mass imbalance calculation and dynamic weight addition.
