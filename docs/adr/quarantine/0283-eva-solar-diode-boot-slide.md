# ADR 0283: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Terminal Protective Silicone Boot Slide

## Status
Accepted

## Context
Following torque calibration of terminal screws, exposed metal contacts on bypass diode cathode lugs must be shielded against solar wind proton bombardment and hypervelocity micrometeoroids outside the Dark Athena. Sliding a high-dielectric silicone boot over the terminal lug collar seals the joint and prevents arcing across adjacent solar array trusses.

## Decision
Implement physical bypass diode terminal protective silicone boot sliding mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the diode lug bracket (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand positions the terminal protective boot applicator (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Sliding Insertion Stroke**: Drive the right hand forward along the terminal lug axis (`rVel.z < -0.80\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` to align the boot collar, slide the elastomeric sleeve over the terminal lug, lock the retaining lip, and seal electrical contacts.
- **Haptic Feedback**: $280\text{ Hz}$ ($0.95$) elastomeric boot detent snap on the dominant controller and $140\text{ Hz}$ ($0.55$) bracket guide tension on the off-hand controller.

## Consequences
- Protects high-amperage terminal joints from dielectric arcing and micrometeoroid erosion during orbital EVA.
- Pairs with lead screw torquing, ceramic wafer tab crimping, and diode clamp springs.
