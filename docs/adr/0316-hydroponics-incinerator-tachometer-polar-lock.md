# ADR 0316: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Vector Phase Angle Polar Display Graticule Lock

## Status
Accepted

## Context
With the reticle crosshair aligned to the optical tachometer trigger threshold, engaging the polar graticule coordinate lock switch projects the vector unbalance magnitude and angular orientation directly onto the oscilloscope circular phosphor grid, calculating the exact radial slot for dynamic counterweight placement.

## Decision
Implement physical tachometer diagnostic oscilloscope polar coordinate graticule lock mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces oscilloscope chassis bezel (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand pinches polar grid rocker toggle switch (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Vertical Toggle Stroke**: Flick dominant hand vertically upward (`rVel.y > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to engage polar display grid, compute dynamic unbalance vector, and project radial balancing coordinates.
- **Haptic Feedback**: $350\text{ Hz}$ ($0.95$) rocker switch toggle detent snap on dominant controller and $130\text{ Hz}$ ($0.50$) chassis grounding vibration on off-hand controller.

## Consequences
- Transforms raw time-domain tachometer waveform into circular polar balancing vector.
- Essential precursor to physical test weight installation on fan impeller periphery.
