# ADR 0328: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Correction Residual Vibration Sensor Zero-Calibration Verification

## Status
Accepted

## Context
Following the staking of the trial counterweight lock-tang, zeroing the piezoelectric casing accelerometer diagnostic channel verifies that rotor residual unbalance has dropped below 0.8 mm/s RMS, clearing the draft fan for full-throttle induced draft operation.

## Decision
Implement physical draft fan residual vibration sensor zero-calibration mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces oscilloscope bezel rim (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand pinches the calibrated rotary zero-trim potentiometer dial (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotational Adjustment Stroke**: Rotate dominant hand clockwise across zero-trim dial (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` to calibrate accelerometer zero datum and clear vibration fault interlock.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.95$) 10-turn precision potentiometer drag and detent lock click on dominant controller and $130\text{ Hz}$ ($0.50$) chassis grounding vibration on off-hand controller.

## Consequences
- Clears vibration fault protection interlocks on induced draft fan motor starter.
- Essential prerequisite to automated incinerator ignition and flue draft regulation.
