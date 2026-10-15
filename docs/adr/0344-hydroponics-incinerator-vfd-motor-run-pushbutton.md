# ADR 0344: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Variable Frequency Drive Motor Run Forward Pushbutton Latch

## Status
Accepted

## Context
With the VFD speed command set (ADR 0336) and acceleration ramp programmed (ADR 0340), initiating fan impeller rotation requires depressing the heavy-duty green 22mm RUN FORWARD pushbutton on the incinerator operator panel, latching the run relay and triggering the soft-start acceleration sequence.

## Decision
Implement physical draft fan motor run forward pushbutton latch mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the operator console rim (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand thumb/palm covers the green RUN pushbutton (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Pushbutton Plunge Stroke**: Plunge dominant hand forcefully forward into panel (`rVel.z < -0.80\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` + `Q` + `Space` to close pilot run contacts and start the 18.5-second motor acceleration sequence.
- **Haptic Feedback**: $390\text{ Hz}$ ($0.98$) industrial spring-loaded pushbutton contact closure snap on dominant controller and $150\text{ Hz}$ ($0.60$) motor run contactor magnetic pull on off-hand controller.

## Consequences
- Commences automated draft fan acceleration to 3450 RPM.
- Establishes continuous negative chamber draft in the hydroponics bio-waste incinerator.
