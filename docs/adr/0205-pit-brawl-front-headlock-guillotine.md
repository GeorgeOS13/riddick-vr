# ADR 0205: Inmate Pit Brawl Standing Front Headlock & Guillotine Neck Crack

## Status
Accepted

## Context
During aggressive clinch exchanges in the inmate pit, Riddick snaps an opponent's head down, traps their neck in a high-elbow front headlock, locks his hands, and arches violently to crush the cervical vertebrae and seal the airway.

## Decision
Implement physical standing front headlock submission in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: From clinch range ($dist < 0.25\text{ m}$), wrap opponent neck in front headlock with off-hand ($leftLocal.y > -0.15\text{ m}$, Left Grip $> 0.65$) and lock hands with dominant grip ($dist < 0.18\text{ m}$, Right Grip $> 0.65$).
- **Arching Spinal Torque**: Violently arch backward and upward ($lVel.y > 1.15\text{ m/s} \land rVel.y > 1.15\text{ m/s}$) to clamp the carotid arteries and hyperextend cervical vertebrae.
- **Synthesized Controls**: Emits `W` + `S` + `C` + `Space` + `LMB` for clinch lock, spinal compression, and lethal cervical submission.
- **Haptic Feedback**: $250\text{ Hz}$ ($0.85$) cervical compression strain and $340\text{ Hz}$ ($0.98$) tracheal/cervical fracture shockwave.

## Consequences
- Expands high-lethality standing grappling mechanics in unarmed pit combat.
- Complements standing arm-triangles, Kimura locks, and collar snap-down combinations.
