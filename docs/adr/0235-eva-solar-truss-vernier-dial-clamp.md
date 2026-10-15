# ADR 0235: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Axial Play Vernier Caliper Dial Gauge Clamping Screw

## Status
Accepted

## Context
When verifying hinge end-play under microgravity conditions, the vernier depth gauge dial must be securely locked to the hinge collar to prevent measurement drift caused by orbital vibrations. Locking the clamping thumbscrew freezes the measurement bezel for precise tolerance recording.

## Decision
Implement physical dial gauge clamping mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces against the vernier mounting collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand pinches the knurled clamping thumbscrew (`twoHandDist < 0.22\text{ m}`, right grip between $0.45$ and $0.85$, right trigger $< 0.35$).
- **Rotational Thumbscrew Clamping**: Apply clockwise rotational twist with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` to lock the dial caliper collar, freeze measurement, and sync calibration.
- **Haptic Feedback**: $290\text{ Hz}$ ($0.95$) knurled brass clamping screw torque stop and $150\text{ Hz}$ ($0.55$) caliper rail contact friction.

## Consequences
- Guarantees drift-free micrometer readings during EVA space walks.
- Complements vernier depth gauge sliding, lock hinge shimming, and cap screw torquing.
