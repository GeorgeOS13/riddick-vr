# ADR 0271: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Ceramic Insulator Wafer Insertion

## Status
Accepted

## Context
During EVA spacewalk maintenance outside the Dark Athena, extreme temperature swings between direct stellar exposure and eclipse cycles degrade electrical insulation between diode cathode terminals and array ground trusses. Inserting a high-dielectric alumina ceramic insulator wafer between the diode flange and mounting plate guarantees dielectric isolation under peak solar array bus voltage.

## Decision
Implement physical bypass diode ceramic insulator wafer insertion mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the diode mounting plate (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns the ceramic wafer applicator (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Wafer Insertion Stroke**: Drive the right hand forward into the mounting slot (`rVel.z < -0.80\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` to align the insulator slot, seat the ceramic wafer, verify dielectric gap, and lock the mounting plate.
- **Haptic Feedback**: $290\text{ Hz}$ ($0.95$) ceramic wafer detent seating snap on the dominant controller and $150\text{ Hz}$ ($0.55$) mounting plate guide tension on the off-hand controller.

## Consequences
- Prevents catastrophic dielectric breakdown across solar collector bypass diodes during peak orbital solar exposure.
- Seamlessly pairs with diode clamp springs, laser weld pyrometer scans, and retention carabiners.
