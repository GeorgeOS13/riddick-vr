# ADR 0347: Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Azimuth Limit Switch Cam Dog Engagement

## Status
Accepted

## Context
On the exterior solar collector truss of the Dark Athena, the primary sun-tracking solar array utilizes dual-axis elevation/pitch and azimuth/yaw harmonic drive gimbals to align high-efficiency photovoltaic panels with stellar radiation vectors. Following manual pitch and yaw clasp unlatching (ADR 0327, ADR 0331) and servo tracking drive enablement (ADR 0343), mechanical azimuth limit switch travel stops must be engaged to prevent array umbilical cable twist and mechanical binding during automatic closed-loop tracking slews.

## Decision
Implement physical two-handed spatial gesture detection for the azimuth limit switch adjustable cam dog engagement in `src/hook/camera_hook.cpp`:
1. **Bracket Collar Stabilization**: Left hand braces azimuth limit switch mounting bracket ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$).
2. **Cam Dog Slide Alignment**: Right hand aligns adjustable alloy cam dog slide shoe ($dist < 0.22\text{ m}$, Right Grip $> 0.55 \land Right Trigger < 0.40$).
3. **Detent Slot Engagement Stroke**: Rapid linear sweep along slot rail into detent stop seat ($|rVel.x| > 0.65\text{ m/s} \land rVel.z < -0.50\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Space'` + `'Tab'` + `'Return'` + `'Q'` (`0x45`, `0x20`, `0x09`, `0x0D`, `0x51`).
5. **Haptic Feedback**: $400\text{ Hz}$ ($0.95$, $120\text{ ms}$) hardened alloy cam dog detent snap on right controller, $160\text{ Hz}$ ($0.55$, $140\text{ ms}$) limit switch bracket resonance on left controller.

## Consequences
- Guarantees authentic physical maintenance workflow for Dark Athena EVA exterior power systems.
- Seamlessly integrates with the flattened Section 5 gesture architecture.
