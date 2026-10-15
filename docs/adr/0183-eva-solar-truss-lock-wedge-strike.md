# ADR 0183: Dark Athena EVA Solar Collector Truss Articulation Lock Wedge Mallet Strike

## Status
Accepted

## Context
On the exterior solar collector trusses of the Dark Athena, heavy structural articulation wedges freeze during thermal transitions between shadow and solar exposure. Riddick must position the heavy EVA mallet or alloy spanner against the wedge collar and deliver a high-velocity downward hammer strike to seat or unseat the locking wedge.

## Decision
Implement physical EVA mallet/wedge strike detection in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position dominant hand at truss articulation bracket ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.42]\text{ m}$) with controller grip firmly held ($Grip > 0.60$).
- **Dynamic Hammer Strike**: Deliver a sharp vertical downward hammer strike ($V_y < -1.20\text{ m/s}$) to drive the lock wedge home.
- **Synthesized Controls**: Emits `E` + `Space` + `LMB` + `Enter` for mechanical wedge seating and structural lock registration.
- **Haptic Feedback**: $270\text{ Hz}$ ($0.98$) metallic wedge percussion impact shockwave, followed by $110\text{ Hz}$ ($0.65$) truss resonant chime.

## Consequences
- Deepens interactive EVA spacewalk repairs and sabotages on the Dark Athena superstructure.
- Fully compatible with zero-gravity EVA locomotion and thruster mechanics.
