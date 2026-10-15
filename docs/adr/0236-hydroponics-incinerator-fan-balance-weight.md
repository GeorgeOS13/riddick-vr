# ADR 0236: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Weight Clip Insertion

## Status
Accepted

## Context
High-temperature particulate erosion on the induced draft fan impeller creates rotational unbalance that rattles the bearing housing. Inserting calibrated balance clips onto the fan blade trailing edges through the inspection hatch stabilizes the rotor assembly and restores quiet exhaust ventilation.

## Decision
Implement physical rotor balance weight clip insertion in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the balance weight guide jig at the fan inspection port (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand aligns the spring-steel balance clip (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Clip Insertion**: Drive the right hand forward with a firm insertion snap (`rVel.z < -0.85\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to seat the clip on the impeller blade rib, crimp the tang, and check clearance.
- **Haptic Feedback**: $280\text{ Hz}$ ($0.95$) spring-steel tang snap on the right controller and $130\text{ Hz}$ ($0.60$) blade runner resonance on the left controller.

## Consequences
- Mitigates induced draft fan vibration across subterranean hydroponics ventilation channels.
- Integrates with draft fan bearing sight-glass venting, plenum cleanout dog clamps, and ash winches.
