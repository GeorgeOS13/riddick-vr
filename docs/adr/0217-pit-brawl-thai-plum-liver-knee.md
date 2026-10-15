# ADR 0217: Inmate Pit Brawl Clinch Thai Plum Collar Tie & Liver Knee Strike

## Status
Accepted

## Context
In the claustrophobic confines of pit fighting, establishing double collar control (the Thai Plum) breaks an opponent's posture, exposing their ribs and liver to catastrophic close-range vertical knee strikes.

## Decision
Implement physical Thai Plum clinch knee strike in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: From clinch range ($dist < 0.25\text{ m}$), clamp both hands behind opponent crown ($leftLocal.y > -0.10\text{ m} \land rightLocal.y > -0.10\text{ m}$, both grips $> 0.65$).
- **Coordinated Down-Pull & Knee Drive**: Pull both hands downward violently ($lVel.y < -1.10\text{ m/s} \land rVel.y < -1.10\text{ m/s}$) to snap the opponent's torso down into an ascending knee strike.
- **Synthesized Controls**: Emits `W` + `C` + `Space` + `LMB` for posture break, liver strike, and concussive rib fracture.
- **Haptic Feedback**: $220\text{ Hz}$ ($0.90$) cervical collar drag tension followed by $320\text{ Hz}$ ($0.98$) concussive liver impact shockwave.

## Consequences
- Broadens authentic Muay Thai clinch combat dynamics in pit brawls.
- Pairs with lead knee thrusts, front headlocks, and clinch headbutts.
