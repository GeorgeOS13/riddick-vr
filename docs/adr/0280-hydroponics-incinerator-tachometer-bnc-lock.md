# ADR 0280: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Amplifier BNC Output Connector Twist-Lock

## Status
Accepted

## Context
Conditioned telemetry signals from the incinerator draft fan optical tachometer amplifier module must be routed through shielded coaxial lines to external diagnostic hardware. Inserting and twisting a bayonet BNC connector into the amplifier output receptacle compresses an internal silicone gasket and locks coaxial ground continuity.

## Decision
Implement physical tachometer amplifier BNC connector twist-lock mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand stabilizes the amplifier BNC port boss (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand aligns the coaxial cable BNC collar (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Insertion & Quarter-Turn Twist**: Drive the right hand forward and rotate clockwise (`rVel.z < -0.70\text{ m/s} \land |rVel.x| > 0.60\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to engage bayonet pins, rotate the outer collar, seat the central pin, and lock coaxial telemetry.
- **Haptic Feedback**: $280\text{ Hz}$ ($0.95$) bayonet detent snap on the dominant controller and $150\text{ Hz}$ ($0.55$) bulkhead receptacle resistance on the off-hand controller.

## Consequences
- Guarantees vibration-resistant, low-noise coaxial telemetry routing to diagnostic balancing equipment.
- Complements heat-shrink sleeve curing, silicone strain relief boots, and magnetic shield wraps.
