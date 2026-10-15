# ADR 0243: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Dial Gauge Retaining Dog Set Screw

## Status
Accepted

## Context
When locking the vernier caliper onto the hinge collar during prolonged spacewalk inspections, microgravity drift can cause the clamping block to loosen. Torquing a secondary dog set screw locks the caliper fixture solidly against the hinge guide lug, preventing loss of calibration during attitude control maneuvers.

## Decision
Implement physical vernier retaining dog set screw mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces against the vernier mounting anvil (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns a hex driver at the retaining dog set screw (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotational Set Screw Torquing**: Apply clockwise rotational wrist torque with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` to seat the dog set screw, clamp the anvil fixture, and lock tolerance readings.
- **Haptic Feedback**: $290\text{ Hz}$ ($0.95$) hardened steel dog screw torque stop on the right hand and $160\text{ Hz}$ ($0.60$) hinge lug friction on the left hand.

## Consequences
- Secures precision measurement fixtures during high-vibration orbital attitude adjustments.
- Complements zero-calibration thumbscrews, dial clamps, and shim insertions.
