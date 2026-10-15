# ADR 0275: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Ceramic Insulator Wafer Lock Tab Plier Crimp

## Status
Accepted

## Context
During EVA spacewalk operations outside the Dark Athena, structural vibration from RCS attitude control thrusters can loosen ceramic insulator wafers seated within diode brackets. Applying crimping pliers across the diode bracket lock tab deforms the retention dog over the wafer edge, staking the ceramic permanently against orbital g-forces and thermal cycling.

## Decision
Implement physical bypass diode ceramic wafer lock tab plier crimp mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the diode mounting plate collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns crimping pliers over the bracket lock tab (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Crimp Squeeze Stroke**: Drive the right hand downward across the tab flange (`rVel.y < -0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` to engage the pliers, crimp the retention tab, stake the ceramic wafer, and lock the housing.
- **Haptic Feedback**: $300\text{ Hz}$ ($0.95$) tab deformation crimp snap on the dominant controller and $160\text{ Hz}$ ($0.55$) bracket clamp tension on the off-hand controller.

## Consequences
- Prevents wafer loosening and preserves dielectric insulation during violent hull maneuvers.
- Integrates seamlessly with wafer insertion, clamp springs, and weld pyrometer scans.
