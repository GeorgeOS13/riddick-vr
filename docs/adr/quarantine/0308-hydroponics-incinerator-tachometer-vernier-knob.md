# ADR 0308: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Timebase Variable Vernier Calibration Knob Twist

## Status
Accepted

## Context
Measuring rotor angular unbalance requires exact horizontal phase graticule scaling. Adjusting the concentric variable timebase vernier knob fine-tunes horizontal sweep time to map one rotor revolution across exactly ten screen divisions (36 degrees per division), enabling direct reading of imbalance mass angular locations.

## Decision
Implement physical tachometer diagnostic oscilloscope timebase variable vernier calibration knob twist mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the oscilloscope side bracket (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand pinches the concentric vernier calibration knob (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Rotary Angular Adjustment Stroke**: Rotate or sweep the right hand laterally across dial circumference (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to grasp timebase vernier, scale horizontal timebase to 36 deg/div, calibrate 1-revolution display sweep, and lock phase angle graticule.
- **Haptic Feedback**: $300\text{ Hz}$ ($0.95$) continuous viscous potentiometer drag on dominant controller and $120\text{ Hz}$ ($0.50$) chassis grounding hum on off-hand controller.

## Consequences
- Maps 360 degrees of fan rotor rotation directly onto the 10-division oscilloscope screen.
- Precedes test weight placement and final dynamic balancing runs.
