# ADR 0247: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Dial Gauge Retaining Dog Set Screw Driver Detent Release

## Status
Accepted

## Context
Following completion of micrometer end-play measurement and logging, the retaining dog fixture holding the dial gauge against the truss hinge collar must be unlocked. Unthreading the dog set screw back into its detent release seat allows smooth removal of the vernier fixture without scratching precision machined bearing races.

## Decision
Implement physical vernier retaining dog set screw detent release mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces against the vernier mounting anvil (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns a hex driver at the retaining dog set screw (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotational Release Torque**: Apply counter-clockwise rotational wrist torque with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` to unseat the dog set screw, release the anvil fixture, and disengage the gauge.
- **Haptic Feedback**: $260\text{ Hz}$ ($0.65$) set screw thread unseating rasp on the right hand and $180\text{ Hz}$ ($0.55$) fixture clamp release pop on the left hand.

## Consequences
- Allows clean, damage-free removal of precision measurement equipment during spacewalks.
- Completes the operational inspection cycle of the EVA solar collector truss articulation hinge.
