# ADR 0291: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Thermal Grease Syringe Bead Extrusion

## Status
Accepted

## Context
Prior to seating replacement bypass diode ceramic wafers into the Dark Athena solar collector busbar pocket, applying a uniform line of zinc-oxide thermal transfer grease via a manual syringe barrel ensures void-free metal-to-ceramic thermal conduction under intense orbital solar radiation.

## Decision
Implement physical bypass diode thermal grease syringe bead extrusion mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand steadies the heatsink diode pocket (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns the syringe applicator nozzle (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Lateral Extrusion Stroke**: Sweep the right hand horizontally across the pocket channel (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Space` to position syringe tip, depress plunger, lay uniform grease bead, and retract nozzle cleanly.
- **Haptic Feedback**: $260\text{ Hz}$ ($0.95$) viscous paste extrusion drag on the dominant controller and $140\text{ Hz}$ ($0.50$) heatsink pocket stabilization on the off-hand controller.

## Consequences
- Guarantees thermal dissipation across diode substrate under full vacuum solar flux.
- Seamlessly precedes ceramic wafer seating, tab crimping, terminal screw torquing, and silicone boot zip-tie retention.
