# ADR 0276: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Optical Tachometer Signal Cable Heat-Shrink Sleeve Heat Gun Pass

## Status
Accepted

## Context
Corrosive pyrolytic acid vapors and unburned hydrocarbon droplets inside the bio-waste incinerator draft blower room attack silicone cable boots. Sweeping an electric heat gun nozzle along a fluoropolymer heat-shrink protective sleeve activates thermoplastic sealant, fusing the sleeve over the boot collar and guaranteeing a hermetic barrier.

## Decision
Implement physical tachometer signal cable heat-shrink sleeve heat gun pass mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand stabilizes the cable boot collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand sweeps the heat gun nozzle (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotational Heat Gun Sweep**: Apply lateral or rotational sweeping motion with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` to activate the heat coil, shrink the sleeve, melt thermoplastic sealant, and certify a hermetic seal.
- **Haptic Feedback**: $250\text{ Hz}$ ($0.95$) hot-air blower buzz on the dominant controller and $150\text{ Hz}$ ($0.55$) curing sleeve contraction tension on the off-hand controller.

## Consequences
- Protects telemetry cable transitions from acidic flue gas corrosion.
- Complements silicone strain relief boots, magnetic shielding, and potentiometer calibrations.
