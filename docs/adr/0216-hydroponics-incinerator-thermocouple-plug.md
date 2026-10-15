# ADR 0216: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Flue Gas Thermocouple Well Threaded Plug Tightening

## Status
Accepted

## Context
High-pressure pyrolytic gas leaks from the incinerator diagnostic pyrometer wells if the protective threaded plugs are loose. Riddick uses an open-end wrench to torque the hexagonal plug into the casing boss, crushing the copper sealing ring.

## Decision
Implement physical thermocouple plug torqueing in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position dominant hand at the casing plug boss ($Z \in [-0.40, -0.16]\text{ m}, Y \in [-0.10, 0.25]\text{ m}$) with wrench grip ($Grip \in [0.45, 0.85] \land Trigger < 0.40$).
- **Torque Twist**: Execute a sharp clockwise rotational wrist twist ($|V_x| > 0.65\text{ m/s}$) to torque the hexagonal plug home.
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` for thread engagement, crush washer compression, and hermetic seal validation.
- **Haptic Feedback**: $160\text{ Hz}$ ($0.55$) copper crush washer deformation friction followed by $270\text{ Hz}$ ($0.98$) metallic torque stop thud.

## Consequences
- Deepens facility puzzle maintenance in the waste reclamation sector.
- Works in tandem with secondary air dampers, primary swirl vanes, and precipitator rapping rods.
