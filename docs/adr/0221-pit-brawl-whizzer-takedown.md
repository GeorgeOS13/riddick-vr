# ADR 0221: Inmate Pit Brawl Double Collar Tie Whizzer Takedown & Arm Trap

## Status
Accepted

## Context
When pit fighting opponents attempt to counter inside or drive for an underhook, clamping an overhook (whizzer) collapses their shoulder plane. Combining this overhook with a violent lateral collar whip rotates the adversary off-balance and spikes them onto the deck.

## Decision
Implement physical whizzer takedown in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: From clinch range ($dist < 0.25\text{ m}$), clamp dominant hand over opponent shoulder ($Y \in [-0.15, 0.15]\text{ m}$, Right Grip $> 0.65$) and off-hand behind opposite collar ($Left Grip > 0.65$).
- **Coordinated Down-Whip**: Drive dominant elbow downward ($rVel.y < -1.15\text{ m/s}$) while whipping off-hand laterally ($|lVel.x| > 0.90\text{ m/s}$) to collapse the posture.
- **Synthesized Controls**: Emits `W` + `A`/`D` + `C` + `Space` + `LMB` for lateral whizzer trip, shoulder trap, and deck slam.
- **Haptic Feedback**: $210\text{ Hz}$ ($0.85$) overhook shoulder lever strain followed by dual $45\text{ Hz}$ ($0.98$) concussive mat impact.

## Consequences
- Broadens authentic freestyle/greco-roman wrestling clinch counters.
- Pairs with Thai plum knees, front headlocks, and suplex slams.
