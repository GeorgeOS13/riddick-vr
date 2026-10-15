# ADR 0335: Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Harmonic Drive Manual Hand-Crank Slew Calibration

## Status
Accepted

## Context
With both pitch (ADR 0327) and yaw (ADR 0331) clasps unlatched, initial optical alignment of the photovoltaic array towards the local solar vector requires manual slew calibration via the auxiliary hand-crank on the harmonic drive input shaft. Rotating this crank advances the high-ratio flexspline to align the array normal with incident stellar radiation.

## Decision
Implement physical solar collector harmonic drive manual hand-crank slew calibration mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the harmonic drive gearbox casing collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand grasps the folding hand-crank knob (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Circular Hand-Crank Slew Stroke**: Rotate dominant hand continuously through the vertical orbital plane (`\sqrt{rVel.y^2 + rVel.z^2} > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Return` + `Q` to advance the flexspline wave generator and lock solar index alignment.
- **Haptic Feedback**: $330\text{ Hz}$ ($0.92$) gear teeth mesh friction and detent click on dominant controller and $150\text{ Hz}$ ($0.55$) harmonic drive casing resonance on off-hand controller.

## Consequences
- Slew-aligns solar array normal vector with incident stellar flux.
- Completes physical mechanical preparation for automated photovoltaic sun-tracking.
