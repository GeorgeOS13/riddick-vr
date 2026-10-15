# ADR 0188: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Soot Blower Lance Oscillation

## Status
Accepted

## Context
In the flue gas economizer section of the bio-waste incinerator, fly ash and pyrolytic soot coat boiler tubes, reducing draft. Riddick operates the high-pressure rotary soot blower lance, inserting it into the tube bank and executing reciprocating linear oscillations while holding the steam blast trigger.

## Decision
Implement physical soot blower lance reciprocating oscillation in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Two-handed stance at economizer port ($dist < 0.28\text{ m}, Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.20, 0.25]\text{ m}$) with both grips clenched ($> 0.55$) and dominant trigger held ($> 0.60$).
- **Reciprocating Lance Strokes**: Execute 3 reciprocating axial strokes ($|\Delta V_z| > 0.65\text{ m/s}$) to clear pyrolytic slag and soot from heat exchangers.
- **Synthesized Controls**: Emits `E` + `Space` + `Q` + `LMB` for steam valve opening, lance cycling, and soot clearance.
- **Haptic Feedback**: $120\text{ Hz}$ ($0.75$) steam jet roar combined with cyclic $220\text{ Hz}$ ($0.55$) tube scraping rasp.

## Consequences
- Enhances industrial facility interaction realism with multi-stroke pneumatic tool dynamics.
- Seamlessly fits into the hydroponics incinerator maintenance network.
