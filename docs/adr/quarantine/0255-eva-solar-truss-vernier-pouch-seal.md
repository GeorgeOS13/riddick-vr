# ADR 0255: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Storage Pouch Magnetic Flap Seal

## Status
Accepted

## Context
After returning the vernier depth caliper fixture into the thigh tool pouch during EVA space walks, sealing the pouch flap prevents tool floatation or snagging on exterior solar array truss guy-wires during high-velocity magnetic boot traversing across the ship hull.

## Decision
Implement physical vernier storage pouch magnetic flap sealing mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces against the thigh holster rim (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $-0.45\text{ m}$ and $-0.10\text{ m}$, left grip $> 0.55$). Right hand grasps the magnetic pouch closure flap (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Downward Sealing Stroke**: Drive the right hand downward across the magnetic flap seam (`rVel.y < -0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` to close the pouch flap, engage neodymium clasp magnets, and lock tool inventory.
- **Haptic Feedback**: $270\text{ Hz}$ ($0.95$) dual neodymium magnetic clasp snap on the right controller and $130\text{ Hz}$ ($0.55$) Kevlar pouch fabric tension on the left controller.

## Consequences
- Concludes the physical EVA tool maintenance workflow for exterior truss assemblies.
- Pairs with vernier caliper fixture removal, dog screw releases, and zero-calibrations.
