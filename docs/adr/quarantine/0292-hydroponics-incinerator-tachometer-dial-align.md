# ADR 0292: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Diagnostic Oscilloscope Frequency Dial Alignment

## Status
Accepted

## Context
With probe tip and ground clip secured to the induced draft fan optical tachometer pickup, adjusting the diagnostic oscilloscope timebase frequency dial synchronizes the pulse train display with fan rotor rotation, enabling precise phase jitter and angular imbalance analysis.

## Decision
Implement physical tachometer diagnostic oscilloscope frequency dial alignment mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the oscilloscope front bezel (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand grasps the rotary timebase frequency knob (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotary Angular Adjustment Stroke**: Rotate or sweep the right hand laterally across the dial radius (`|rVel.x| > 0.70\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` to engage dial detent, step sweep scale to 10us/div, lock trigger sweep, and center tachometer pulse waveform.
- **Haptic Feedback**: $320\text{ Hz}$ ($0.95$) 12-position detented rotary switch clicks on the dominant controller and $120\text{ Hz}$ ($0.50$) oscilloscope chassis fan rumble on the off-hand controller.

## Consequences
- Locks oscilloscope sweep rate to rotor harmonic frequency for real-time balancing.
- Pairs directly with probe ground clipping, BNC twist-locking, and dynamic counterweight placement.
