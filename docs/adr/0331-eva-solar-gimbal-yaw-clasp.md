# ADR 0331: Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Yaw Actuator Manual Clasp Unlatch

## Status
Accepted

## Context
Following the release of the elevation pitch clasp (ADR 0327), liberating the solar array for full two-axis heliostatic sun-tracking requires unlatching the secondary manual lock clasp on the azimuth yaw yoke actuator. This disengages the mechanical parking caliper and restores automated azimuth rotation.

## Decision
Implement physical solar collector gimbal yaw actuator lock clasp unlatch mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the gimbal yoke frame collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand grasps the spring-loaded yaw clasp handle (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Lateral Unlatch Stroke**: Haul dominant hand outward and rearward (`|rVel.x| > 0.75\text{ m/s} \land rVel.z > 0.50\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Space` + `Q` to pop the azimuth locking pawl and swing the yaw clasp into open position.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.95$) hardened titanium yaw clasp toggle snap on dominant controller and $145\text{ Hz}$ ($0.52$) gimbal yoke low-frequency resonance on off-hand controller.

## Consequences
- Unlocks azimuth yaw axis of solar array tracking gimbal.
- Fully frees solar wing for automated photovoltaic power capture across all orbital inclinations.
