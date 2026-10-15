# ADR 0296: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Trigger Threshold Level Knob Twist

## Status
Accepted

## Context
With frequency sweep locked on the diagnostic oscilloscope, adjusting the trigger threshold level rotary potentiometer eliminates waveform jitter and phase ambiguity by locking oscilloscope triggering to the leading nanosecond edge of the optical tachometer pulse.

## Decision
Implement physical tachometer diagnostic oscilloscope trigger threshold level knob twist mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the oscilloscope side chassis bracket (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand grasps the trigger level rotary knob (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Rotary Angular Adjustment Stroke**: Rotate or sweep the right hand laterally across the knob circumference (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to grasp trigger potentiometer, increment comparator threshold voltage, engage trigger lock LED indicator, and stabilize optical pulse trace.
- **Haptic Feedback**: $310\text{ Hz}$ ($0.95$) micro-stepped trigger potentiometer clicks on dominant controller and $130\text{ Hz}$ ($0.50$) fan induction motor chassis harmonics on off-hand controller.

## Consequences
- Stabilizes pulse trace jitter under extreme draft fan turbulence.
- Seamlessly pairs with BNC ground clips, sweep frequency tuning, and rotor balancing runs.
