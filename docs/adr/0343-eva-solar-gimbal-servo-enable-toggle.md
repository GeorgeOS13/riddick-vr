# ADR 0343: Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Closed-Loop Optical Tracking Servo Controller Enable Switch Toggle

## Status
Accepted

## Context
With the 4-quadrant photodiode collimator aligned (ADR 0339), activating autonomous closed-loop sun-tracking requires throwing the sealed heavy-duty toggle switch on the gimbal servo drive enclosure from STANDBY to CLOSED-LOOP AUTO, energizing the brushless servo drive amplifiers for automated azimuth and elevation heliostatic positioning.

## Decision
Implement physical solar collector closed-loop servo controller enable switch toggle mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the servo electronics enclosure collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand pinches the toggle switch lever (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Upward Flip Stroke**: Throw dominant hand upward and forward (`rVel.y > 0.65\text{ m/s} \land rVel.z < -0.40\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` + `Space` to close servo enable interlock contacts and initialize closed-loop PID heliostatic tracking.
- **Haptic Feedback**: $380\text{ Hz}$ ($0.98$) spring-loaded toggle switch detent snap on dominant controller and $170\text{ Hz}$ ($0.60$) servo drive inverter coil whine on off-hand controller.

## Consequences
- Engages autonomous closed-loop sun-tracking on Dark Athena solar array.
- Ensures peak photovoltaic power generation during all orbital solar windows.
