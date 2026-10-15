# ADR 0204: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Ash Quenching Trough Agitator Rake Sweep

## Status
Accepted

## Context
Quenched bottom ash and vitrified slag clinkers accumulate in the water seal trough beneath the bio-waste incinerator, threatening to choke the sludge extraction pump. Riddick grasps the manual agitator rake with both hands and performs reciprocating sweeps across the trough floor to break clinkers into pumpable slurry.

## Decision
Implement physical two-handed ash trough rake sweeps in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position hands at the trough rake shaft ($dist \in [0.22, 0.45]\text{ m}, Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.45, -0.15]\text{ m}$) with both grips clenched ($> 0.55$).
- **Reciprocating Sweeps**: Execute alternating lateral scraping sweeps ($|\Delta V_x| > 0.70\text{ m/s}$) to break vitrified ash clinkers.
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` for slurry agitation, clinker fragmentation, and sump pump intake clearance.
- **Haptic Feedback**: $180\text{ Hz}$ ($0.65$) gritty slurry grating friction and $280\text{ Hz}$ ($0.95$) clinker fracture crunch.

## Consequences
- Enhances environmental puzzle and fluid mechanics in the waste recycling subsystem.
- Works alongside slag tap oxygen lances, sludge siphon pumps, and steam purge scrubbers.
