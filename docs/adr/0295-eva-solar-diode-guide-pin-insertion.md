# ADR 0295: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Ceramic Wafer Alignment Guide Pin Insertion

## Status
Accepted

## Context
Before securing terminal lead screws over newly greased heatsink pockets on the Dark Athena solar collector array, inserting precision alignment guide pins through the ceramic bypass diode wafer locating bores ensures perfect concentricity with copper busbar studs without cracking delicate ceramic substrates.

## Decision
Implement physical bypass diode ceramic wafer alignment guide pin insertion mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand steadies the heatsink diode pocket collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns the precision guide pin (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Linear Forward Insertion Stroke**: Drive the right hand forward along the guide bore axis (`rVel.z < -0.75\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` to position guide pin, slide through ceramic wafer bore, engage busbar locating hole, and verify mechanical centering.
- **Haptic Feedback**: $290\text{ Hz}$ ($0.95$) polished dowel pin sliding stop click on dominant controller and $150\text{ Hz}$ ($0.50$) heatsink pocket grounding tension on off-hand controller.

## Consequences
- Eliminates mechanical shear stress on ceramic diode wafers during high-vibration engine burns.
- Precedes terminal screw torquing, silicone boot sliding, and zip-tie cinching.
