# ADR 0200: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Electrostatic Precipitator Rapping Rod Impact

## Status
Accepted

## Context
Combustion particulates from bio-waste incineration foul the high-voltage electrostatic precipitator plates, degrading exhaust filtration. Riddick grips the mechanical rapping rod and drives it against the external anvil plate to dislodge accumulated fly ash.

## Decision
Implement physical two-handed precipitator rapping rod impact in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position hands on the rapping rod handle ($dist < 0.28\text{ m}, Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.20, 0.25]\text{ m}$) with both grips clenched ($> 0.60$).
- **Impact Plunge**: Deliver a forceful forward axial thrust ($rVel.z < -1.15\text{ m/s}$) impacting the rapping rod against the precipitator plate anvil.
- **Synthesized Controls**: Emits `E` + `Space` + `Q` + `Tab` for mechanical vibration transfer, fly ash shedding, and flue pressure restoration.
- **Haptic Feedback**: $270\text{ Hz}$ ($0.98$) rapping anvil impact shockwave accompanied by $85\text{ Hz}$ ($0.70$) plate vibration reverberation.

## Consequences
- Expands environmental industrial maintenance and puzzle solving in the lower recycling decks.
- Complements soot blowers, steam lances, and primary swirl vane mechanics.
