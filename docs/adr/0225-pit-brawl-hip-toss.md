# ADR 0225: Inmate Pit Brawl Double Underhook Body Lock Foot Sweep & Hip Toss

## Status
Accepted

## Context
During aggressive close-range pit brawl exchanges, slipping double underhooks gives complete control of an opponent's center of mass. Combining an upward thoracic hoist with a rotational hip throw flings the opponent across Riddick's hip into the floor grating.

## Decision
Implement physical hip toss in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: From tight clinch range ($dist < 0.22\text{ m}$), wrap both arms into double underhooks ($Y \in [-0.20, 0.15]\text{ m}$, both grips $> 0.65$).
- **Rotational Hip Toss Execution**: Execute explosive lateral torso rotation ($|\Delta V_x| > 1.20\text{ m/s}$) while lifting upward ($rVel.y > 1.05\text{ m/s} \lor lVel.y > 1.05\text{ m/s}$) to launch the hip toss.
- **Synthesized Controls**: Emits `W` + `A`/`D` + `C` + `Space` + `LMB` for hip elevation, ankle reap, and rotational deck throw.
- **Haptic Feedback**: $230\text{ Hz}$ ($0.90$) thoracic squeeze lift tension followed by dual $40\text{ Hz}$ ($0.98$) hip toss deck impact blast.

## Consequences
- Elevates authentic high-amplitude wrestling throws in pit combat.
- Pairs with whizzer takedowns, belly-to-back suplexes, and fireman's carries.
