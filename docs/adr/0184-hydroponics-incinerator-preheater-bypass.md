# ADR 0184: Subterranean Hydroponics Bio-Waste Incinerator Secondary Air Preheater Bypass Flap

## Status
Accepted

## Context
In the hydroponics bio-waste incinerator lower ducting, secondary combustion air preheaters clog during pyrolytic flare-ups. Riddick must reach to the preheater bypass housing with both hands, grasp the dual toggle pins, and yank horizontally rearward to trip the emergency preheater bypass damper.

## Decision
Implement physical dual-hand preheater bypass flap release in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position both hands at the preheater bypass housing ($dist \in [0.22, 0.45]\text{ m}, Z \in [-0.40, -0.16]\text{ m}, Y \in [-0.20, 0.20]\text{ m}$) with both grips clenched ($> 0.55$).
- **Bypass Haul**: Haul horizontally rearward simultaneously ($V_{z, left} > 0.75\text{ m/s}, V_{z, right} > 0.75\text{ m/s}$) to trip bypass flap latches.
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` for damper trip and pneumatic bypass equalization.
- **Haptic Feedback**: Dual $160\text{ Hz}$ ($0.70$) sliding damper friction drag and $250\text{ Hz}$ ($0.95$) mechanical bypass seat thud.

## Consequences
- Expands incinerator facility interaction chains and hazardous ventilation puzzles.
- Bridges naturally with existing incinerator systems (blast door levers, damper wheel, sight-glass, scrubber drain, purge lance).
