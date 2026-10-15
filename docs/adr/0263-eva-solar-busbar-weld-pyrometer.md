# ADR 0263: Dark Athena EVA Solar Collector Photovoltaic Cell Busbar Laser Weld Inspection Pyrometer

## Status
Accepted

## Context
During EVA spacewalk maintenance outside the Dark Athena, laser welds across photovoltaic cell busbars must be verified for micro-cracks and thermal conductivity before solar flare exposure. Holding an infrared optical pyrometer probe flush against the busbar seam with the off-hand while driving the laser pyrometer scan head forward with the dominant hand registers thermal emissivity and confirms weld integrity.

## Decision
Implement physical photovoltaic busbar laser weld inspection pyrometer mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the busbar reference guide (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns the pyrometer scan head (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Pyrometer Scan Stroke**: Drive the right hand forward along the weld seam (`rVel.z < -0.80\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` to align the pyrometer, activate the laser targeting reticle, measure emissivity, and commit the inspection log.
- **Haptic Feedback**: $310\text{ Hz}$ ($0.95$) laser pyrometer pulse chime on the dominant controller and $160\text{ Hz}$ ($0.55$) busbar thermal contact buzz on the off-hand controller.

## Consequences
- Guarantees solar array busbar structural integrity and electrical efficiency prior to orbital solar flare impact.
- Complements retention carabiners, pouch sealing, and dial gauge calibrations.
