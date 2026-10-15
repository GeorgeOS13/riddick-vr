# ADR 0311: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Silicone Insulator Boot Final Slide & Hermetic Seal Clench

## Status
Accepted

## Context
With the bypass diode cathode terminal lug torqued to specification, sliding the heavy-duty molded fluorosilicone protective boot over the terminal post and applying a firm two-handed clench seals the terminal junction against orbital micrometeorite dust, vaporized propellant residue, and space plasma deposition.

## Decision
Implement physical bypass diode silicone boot final slide and hermetic seal clench mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the diode bracket collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand grasps the silicone boot collar (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Forward Slide & Clench Stroke**: Drive the right hand forward along the terminal lug axis (`rVel.z < -0.80\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` to seat the boot, clench hermetic lip seal over terminal flange, and lock elastomeric detent.
- **Haptic Feedback**: $290\text{ Hz}$ ($0.95$) elastomeric seal seating pop on dominant controller and $150\text{ Hz}$ ($0.50$) heatsink bracket reaction on off-hand controller.

## Consequences
- Hermetically protects bypass diode electrical terminals against orbital vacuum environment.
- Precedes final nylon zip-tie tensioning and flush cut.
