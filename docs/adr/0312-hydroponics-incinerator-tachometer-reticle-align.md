# ADR 0312: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Phase Marker Reticle Crosshair Alignment

## Status
Accepted

## Context
Following timebase vernier horizontal sweep calibration, adjusting the dual-axis position controls aligns the oscilloscope illuminated reticle crosshair precisely on the 50% rise amplitude of the optical tachometer pulse, setting an absolute 0-degree rotor phase angle reference for dynamic balance mass addition.

## Decision
Implement physical tachometer diagnostic oscilloscope reticle crosshair alignment mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the oscilloscope lower chassis bezel (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand pinches the dual-axis position joystick/knob (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Coordinated Diagonal Alignment Stroke**: Sweep the right hand diagonally across the position control plane (`|rVel.x| > 0.65\text{ m/s} \land |rVel.y| > 0.45\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` to position graticule crosshairs, lock optical pulse 50% trigger threshold, and display dynamic unbalance phase angle.
- **Haptic Feedback**: $340\text{ Hz}$ ($0.95$) reticle alignment lock detent click on dominant controller and $130\text{ Hz}$ ($0.50$) chassis resonance hum on off-hand controller.

## Consequences
- Locks 0-degree angular reference for fan rotor imbalance measurements.
- Direct prerequisite for precision dynamic counterweight calculation and attachment.
