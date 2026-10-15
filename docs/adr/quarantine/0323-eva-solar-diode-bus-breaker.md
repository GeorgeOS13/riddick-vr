# ADR 0323: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode High-Current DC Bus Main Breaker Switch Interlock Clench

## Status
Accepted

## Context
With electrical continuity confirmed by multimeter probing, closing the high-current DC bus main breaker knife-switch and clenching its mechanical interlock latch energizes the reconstructed solar array wing, restoring primary power to Dark Athena's secondary shield grid.

## Decision
Implement physical bypass diode high-current DC bus breaker knife-switch interlock clench mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand stabilizes the breaker housing collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand grasps the high-voltage insulated knife-switch handle (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Downward Power Throw**: Throw right hand violently downward across switch travel (`rVel.y < -0.80\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Return` + `Q` to close copper knife contacts, clench safety interlock, and energize array DC bus.
- **Haptic Feedback**: $380\text{ Hz}$ ($0.98$) magnetic blowout arc shockwave on dominant controller and $160\text{ Hz}$ ($0.60$) busbar hum on off-hand controller.

## Consequences
- Fully powers reconstructed solar array photovoltaic bank.
- Direct prerequisite to subsequent gimbal drive articulation and solar tracking initialization.
