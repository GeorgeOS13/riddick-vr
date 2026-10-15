# ADR 0339: Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Optical Sun-Sensor Quadrant Photodiode Collimator Alignment

## Status
Accepted

## Context
Following the manual hand-crank slew calibration (ADR 0335), enabling autonomous closed-loop sun tracking requires aligning the 4-quadrant photodiode optical sun-sensor collimator barrel on the solar array outboard spar to center the incident solar disk within the optical reticle mask.

## Decision
Implement physical solar collector optical sun-sensor collimator alignment mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the optical sensor mounting barrel collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand pinches the knurled collimator azimuth lock ring (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Linear Feed & Radial Twist Stroke**: Push forward while twisting horizontally (`rVel.z < -0.75\text{ m/s} \land |rVel.x| > 0.50\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` + `Space` to lock the collimator optical reticle and initialize closed-loop solar tracking servos.
- **Haptic Feedback**: $370\text{ Hz}$ ($0.95$) optical collimator barrel lock detent snap on dominant controller and $160\text{ Hz}$ ($0.55$) outboard spar structural resonance on off-hand controller.

## Consequences
- Centers quadrant photodiode differential tracking voltage within nominal deadband.
- Enables autonomous solar vector tracking across all orbital passes.
