# ADR 0239: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Dial Gauge Zero Calibration Thumbscrew

## Status
Accepted

## Context
Thermal expansion across solar array structural members can induce gauge zero-drift. Before recording precise micrometer end-play along the lock hinge assembly, the technician or Riddick must zero-reference the vernier caliper against a flat calibration reference anvil using a fine-pitch zero calibration thumbscrew.

## Decision
Implement physical dial gauge zero calibration thumbscrew mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the caliper reference anvil at the hinge collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand grasps the knurled zero-adjust thumbscrew on the gauge dial rim (`twoHandDist < 0.22\text{ m}`, right grip between $0.45$ and $0.85$, right trigger $< 0.35$).
- **Rotational Zero-Adjustment**: Apply a counter-clockwise rotational wrist twist with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` to zero-set the dial gauge, reset micrometer registers, and confirm datum lock.
- **Haptic Feedback**: $240\text{ Hz}$ ($0.50$) knurled brass zero-detent ratchet micro-clicks on the right controller and $320\text{ Hz}$ ($0.90$) optical zero-point datum confirmation tone on the left controller.

## Consequences
- Ensures micrometer measurement fidelity across extreme thermal gradients on EVA space walks.
- Pairs with dial clamping, vernier depth sliding, and shimming washer insertion.
