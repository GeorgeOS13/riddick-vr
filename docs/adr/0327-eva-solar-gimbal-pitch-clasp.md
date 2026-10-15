# ADR 0327: Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Pitch Actuator Manual Clasp Unlatch

## Status
Accepted

## Context
With the DC main bus energized, preparing the solar wing for full heliostatic tracking requires unlatching the heavy-duty manual lock clasp on the primary elevation gimbal pitch actuator, freeing the harmonic drive gearbox for automated solar alignment.

## Decision
Implement physical solar collector gimbal pitch actuator lock clasp unlatch mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the gimbal clevis frame (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand pinches the forged spring-loaded pitch lock clasp handle (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Angular Pull & Unlatch Stroke**: Haul dominant hand rearward and upward (`rVel.z > 0.75\text{ m/s} \land rVel.y > 0.50\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Space` + `Q` to pop the safety pawl and swing the pitch lock clasp into open retention.
- **Haptic Feedback**: $350\text{ Hz}$ ($0.95$) hardened steel lock clasp release snap on dominant controller and $140\text{ Hz}$ ($0.50$) gimbal frame resonance on off-hand controller.

## Consequences
- Unlocks primary elevation pitch axis of solar wing gimbal.
- Direct prerequisite to subsequent azimuth yaw clasp unlatching and automated solar tracking.
