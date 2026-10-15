# ADR 0340: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Variable Frequency Drive Acceleration Ramp Parameter Keypad Entry

## Status
Accepted

## Context
With the VFD speed setpoint potentiometer calibrated (ADR 0336), programming the soft-start acceleration ramp rate ($P102 = 18.5\text{ s}$) on the VFD digital operator keypad ensures controlled motor acceleration without triggering overcurrent protection faults on the 480V substation bus.

## Decision
Implement physical draft fan VFD acceleration parameter keypad entry mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the VFD keypad bezel rim (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand points index finger forward towards numeric membrane keypad (`twoHandDist < 0.22\text{ m}`, right grip $< 0.45$, right trigger $< 0.35$).
- **Linear Keypad Poke Stroke**: Deliver linear forward poking stroke (`rVel.z < -0.70\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` + `Space` to enter program mode, store $18.5\text{ s}$ ramp time, and arm drive acceleration curve.
- **Haptic Feedback**: $380\text{ Hz}$ ($0.95$) membrane switch tactile snap on dominant controller and $130\text{ Hz}$ ($0.50$) chassis enclosure resonance on off-hand controller.

## Consequences
- Configures 18.5-second soft-start acceleration curve for induced draft fan.
- Eliminates motor starting inrush transients during bio-waste incineration cycles.
