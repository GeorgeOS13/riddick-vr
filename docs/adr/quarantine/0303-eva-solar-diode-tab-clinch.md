# ADR 0303: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Ceramic Wafer Lock Tab Plier Final Clinch

## Status
Accepted

## Context
Following guide pin retraction and retention latch seating over the replacement bypass diode ceramic wafer, applying a final high-leverage clinch stroke across the phosphor-bronze lock tabs permanently deforms the retention flanges over the ceramic substrate shoulder, ensuring positive mechanical retention against severe vibration during orbital thruster burns.

## Decision
Implement physical bypass diode ceramic wafer lock tab plier final clinch mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand stabilizes the diode pocket bracket (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand positions compound-action clinch plier jaws (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Downward Clinch Stroke**: Drive the right hand downward across the tab plane (`rVel.y < -0.70\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` to position plier jaws, apply 120N clinch force, deform lock tabs over ceramic bevel, and verify retention closure.
- **Haptic Feedback**: $310\text{ Hz}$ ($0.95$) metal tab deformation crunch on dominant controller and $160\text{ Hz}$ ($0.55$) bracket reaction force on off-hand controller.

## Consequences
- Permanently locks diode ceramic wafer in place prior to silicone boot installation.
- Pairs directly with guide pin retraction, terminal screw torquing, and boot slide operations.
