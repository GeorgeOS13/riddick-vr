# ADR 0287: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Silicone Boot Retention Zip-Tie Tension & Flush Cut

## Status
Accepted

## Context
During high-g maneuvering or RCS thruster firing outside the Dark Athena, protective silicone terminal boots seated over bypass diode cathode lugs can loosen. Cinched fluoropolymer ETFE cable ties around the boot neck provide secondary mechanical retention against extreme thermal shock and orbital acceleration.

## Decision
Implement physical bypass diode silicone boot retention zip-tie tension and flush cut mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the diode boot collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand positions the tensioner plier tool (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Downward Tension & Cut Stroke**: Drive the right hand downward across the tie clamp plane (`rVel.y < -0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` to align the plier jaw, cinch the ETFE tie, tension to 80N, and shear the tail flush.
- **Haptic Feedback**: $310\text{ Hz}$ ($0.95$) flush-cut blade snap on the dominant controller and $160\text{ Hz}$ ($0.55$) boot compression tension on the off-hand controller.

## Consequences
- Prevents silicone terminal boot slippage under high orbital g-loads.
- Complements boot sliding, terminal lead screw torquing, and ceramic wafer tab crimping.
