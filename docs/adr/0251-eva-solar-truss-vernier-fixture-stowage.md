# ADR 0251: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Fixture Removal & Stowage

## Status
Accepted

## Context
With hinge end-play measurement completed and verified against Starbreeze engineering tolerances, the technician or Riddick must dismount the vernier caliper fixture from the hinge anvil and slide it safely into the EVA spacesuit thigh tool holster, preventing loose tool orbital drift.

## Decision
Implement physical vernier fixture removal and stowage mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces against the hinge anvil collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand grips the vernier caliper chassis (`twoHandDist < 0.22\text{ m}`, right grip $> 0.60$, right trigger $> 0.50$).
- **Linear Removal Haul**: Haul the right hand rearward away from the mounting lug (`rVel.z > 0.85\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` to disengage the caliper fixture, stow the tool, and seal the hinge casing.
- **Haptic Feedback**: $290\text{ Hz}$ ($0.95$) magnetic anvil quick-release detent clack on the right controller and $150\text{ Hz}$ ($0.55$) hinge guide lug clearance on the left controller.

## Consequences
- Concludes the extensive EVA solar collector truss articulation hinge diagnostic and maintenance protocol.
- Integrates seamlessly with all prior lock ring, shimming, vernier, and cap screw interactions.
