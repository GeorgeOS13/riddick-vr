# ADR 0307: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Cathode Terminal Lug Lead Torque Wrench Tighten

## Status
Accepted

## Context
Following ceramic wafer lock tab clinching, torquing the heavy nickel-plated copper cathode terminal lug with a calibrated torque wrench to 8.5 N·m ensures minimal electrical contact resistance, preventing ohmic heating and high-current arcing across the busbar junction during full orbital solar collection.

## Decision
Implement physical bypass diode cathode terminal lug torque wrench tightening mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand stabilizes the diode cathode terminal post collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns the calibrated torque wrench socket (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotary Angular Torque Stroke**: Rotate or sweep the right hand clockwise across the terminal lug plane (`|rVel.x| > 0.70\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Return` to mate torque wrench socket, torque terminal lug to 8.5 N·m, trip break-over mechanism, and verify electrical contact.
- **Haptic Feedback**: $320\text{ Hz}$ ($0.95$) mechanical torque wrench break-over click on dominant controller and $150\text{ Hz}$ ($0.55$) terminal post reaction tension on off-hand controller.

## Consequences
- Guarantees low-impedance electrical coupling across high-voltage photovoltaic bypass circuits.
- Precedes final silicone boot slide and retention zip-tie installation.
