# ADR 0231: Dark Athena EVA Solar Collector Truss Articulation Drive Motor Pinion Gear Backlash Adjustment

## Status
Accepted

## Context
Thermal distortion across the solar collector truss exterior causes gear mesh tight spots or excessive tooth lash in the main articulation azimuth drive. Fine-tuning the eccentric pinion gear bushing backlash ensures smooth collector tracking without mechanical binding during orbital stationkeeping.

## Decision
Implement physical pinion gear backlash adjustment in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces against the drive motor reduction casing (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand applies the eccentric adjuster socket tool (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Rotary Backlash Adjustment**: Apply angular torque stroke with the right controller (`|rVel.x| > 0.45\text{ m/s} \lor |rVel.y| > 0.45\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` to seat the eccentric adjuster collar, unlock detents, and lock the calibrated mesh clearance.
- **Haptic Feedback**: $280\text{ Hz}$ ($0.92$) eccentric adjuster detent click on the right controller and $140\text{ Hz}$ ($0.60$) gearbox casing resonance on the left controller.

## Consequences
- Provides complete mechanical articulation tuning for exterior truss maintenance sequences.
- Pairs seamlessly with lock hinge axial shimming, retaining cap screw torquing, and thermal blanket sealing flaps.
