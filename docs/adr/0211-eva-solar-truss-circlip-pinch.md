# ADR 0211: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Retaining Ring Circlip Pliers Pinch

## Status
Accepted

## Context
Securing the pivot hinge pin on the solar collector articulation truss against axial drift during orbital pitch maneuvers requires snapping a hardened spring steel circlip retaining ring into the external shaft groove. Riddick stabilizes the collar with his off-hand and uses circlip pliers with his dominant hand to compress and seat the ring.

## Decision
Implement physical circlip pliers seating in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position off-hand at the hinge retaining collar ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$) and dominant hand holding circlip pliers ($dist < 0.22\text{ m}$, Right Grip $> 0.60$, Right Trigger $> 0.60$).
- **Linear Axial Seating Thrust**: Drive the compressed circlip forward along the shaft axis ($rVel.z < -0.85\text{ m/s}$) to snap it into the retention groove.
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` for collar stabilization, ring alignment, and groove seating.
- **Haptic Feedback**: $200\text{ Hz}$ ($0.65$) pliers spring compression tension followed by $300\text{ Hz}$ ($0.98$) circlip snap detent.

## Consequences
- Expands precision mechanical tools during spacewalk operations.
- Complements drift pins, lock wedges, and grease nipple servicing.
