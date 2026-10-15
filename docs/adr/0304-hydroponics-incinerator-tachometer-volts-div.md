# ADR 0304: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Volts/Div Vertical Sensitivity Rotary Switch Step

## Status
Accepted

## Context
With input coupling toggled to AC, the optical pulse amplitude on the diagnostic oscilloscope may clip the vertical graticule boundaries. Stepping the Volts/Div vertical sensitivity rotary attenuator scales the pulse envelope to occupy exactly 4 screen divisions, optimizing TTL edge transition resolution for precision rotor imbalance phase detection.

## Decision
Implement physical tachometer diagnostic oscilloscope Volts/Div vertical sensitivity rotary switch step mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the oscilloscope upper chassis bezel (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand grasps the Volts/Div rotary knob (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotary Angular Adjustment Stroke**: Sweep or rotate the right hand laterally across dial circumference (`|rVel.x| > 0.70\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to grasp Volts/Div knob, step attenuator to 1V/div, lock vertical gain, and frame full pulse envelope.
- **Haptic Feedback**: $330\text{ Hz}$ ($0.95$) 8-position detented rotary switch clicks on dominant controller and $130\text{ Hz}$ ($0.50$) chassis frame vibration on off-hand controller.

## Consequences
- Optimizes waveform vertical scaling for nanosecond-accurate phase jitter measurements.
- Seamlessly pairs with coupling mode toggles, trigger threshold tuning, and fan balance adjustments.
