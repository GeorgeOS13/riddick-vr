# ADR 0284: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Amplifier BNC Cable P-Clamp Screw

## Status
Accepted

## Context
High-frequency mechanical vibration from the induced draft fan motor can fatigue the coaxial telemetry cable at the amplifier BNC connector neck. Securing a cushioned stainless steel P-clamp around the coaxial cable and torquing its mounting screw into the amplifier junction box bracket relieves mechanical stress and eliminates connector wiggle.

## Decision
Implement physical tachometer amplifier BNC cable P-clamp screw mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the cable P-clamp saddle against the junction box flange (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand aligns a hex driver at the P-clamp mounting screw (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotational Torquing Action**: Apply clockwise rotational wrist torque with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` to align the driver, torque the P-clamp screw, compress the elastomer cushion, and isolate the coaxial cable from motor vibration.
- **Haptic Feedback**: $290\text{ Hz}$ ($0.95$) P-clamp torque stop click on the dominant controller and $150\text{ Hz}$ ($0.55$) cushioned saddle compression on the off-hand controller.

## Consequences
- Prevents coaxial fatigue failure and guarantees continuous noise-free telemetry under severe blower vibration.
- Complements BNC twist-locks, heat-shrink sleeve curing, and silicone strain relief boots.
