# ADR 0259: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Storage Pouch Retention D-Ring Carabiner Clip

## Status
Accepted

## Context
During EVA spacewalks outside the Dark Athena, secondary tethering of critical measuring kits is required by spaceport safety protocols. Clipping a spring-loaded retention carabiner through the vernier storage pouch D-ring secures the kit against the suit harness, preventing pouch detachment during high-thrust magnetic boot transitions.

## Decision
Implement physical vernier storage pouch retention carabiner clip mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the pouch D-ring eyelet (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $-0.45\text{ m}$ and $-0.10\text{ m}$, left grip $> 0.55$). Right hand aligns the harness carabiner gate (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Carabiner Snapping Stroke**: Drive the right hand forward through the D-ring loop (`rVel.z < -0.80\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` to depress the spring gate, latch the D-ring, and lock the tether.
- **Haptic Feedback**: $290\text{ Hz}$ ($0.98$) spring-steel carabiner gate snap on the right controller and $140\text{ Hz}$ ($0.60$) titanium D-ring tension on the left controller.

## Consequences
- Guarantees fail-safe tethering of precision instrumentation during high-speed hull locomotion.
- Pairs with magnetic flap sealing, fixture stowage, and retaining dog screw releases.
