# ADR 0279: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Terminal Lead Screw Torquing

## Status
Accepted

## Context
During EVA spacewalk operations outside the Dark Athena, copper busbar leads routed to bypass diode cathode lugs must be fastened to exact torque specifications. Loose terminal screws cause high contact resistance, arcing, and thermal runaway under 100A photovoltaic array current surges during full orbital solar exposure.

## Decision
Implement physical bypass diode terminal lead screw torquing mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the diode lug collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns a hex driver at the lead terminal screw (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotational Torquing Action**: Apply clockwise rotational wrist torque with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` to align the driver, torque the terminal screw, compress the Belleville washer, and establish a low-resistance bond.
- **Haptic Feedback**: $290\text{ Hz}$ ($0.95$) screw torque stop click on the dominant controller and $160\text{ Hz}$ ($0.55$) copper lug tension on the off-hand controller.

## Consequences
- Eliminates contact resistance and prevents terminal arcing under heavy solar array generation.
- Pairs with ceramic wafer tab crimping, diode clamp springs, and weld pyrometer scans.
