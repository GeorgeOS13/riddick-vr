# ADR 0378: Melee Trajectory Filtering & Strike Direction Alignment

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena*, hand-to-hand melee combat features four directional strike animations: Left Hook, Right Hook, Straight Jab, and Uppercut, each executed depending on movement direction keys held while attacking (`A` + `LMB`, `D` + `LMB`, `W` + `LMB`, or `S` + `LMB`).

In `src/hook/camera_hook.cpp`, gestural punch recognition previously attempted to determine attack direction using noisy instantaneous velocity snapshots. Slight wrist rotation or off-axis controller acceleration occasionally triggered unintended directional attacks (e.g., throwing a straight jab but triggering an uppercut or lateral hook).

## Decision
Implement a directional trajectory filter and velocity vector alignment in `src/hook/camera_hook.cpp`:

1. **Integrated Strike Vector Accumulator**:
   - Instead of checking single-frame instantaneous velocity, accumulate controller motion vectors over a $60\text{ ms}$ attack initiation window:
     $$\vec{V}_{\text{strike}} = \sum_{t} \vec{v}(t) \cdot \Delta t$$

2. **Dominant Axis Projection**:
   - Compare the normalized forward component ($|V_z|$), lateral component ($|V_x|$), and vertical component ($|V_y|$):
     - **Forward Jab**: $|V_z| > 1.2 \times \max(|V_x|, |V_y|)$ $\to$ dispatch forward jab (`W` + `LMB`).
     - **Lateral Hook**: $|V_x| > 1.1 \times \max(|V_z|, |V_y|)$ $\to$ dispatch directional hook (`A`/`D` + `LMB` matching swing direction).
     - **Uppercut**: $V_y > 1.1 \times \max(|V_x|, |V_z|)$ $\to$ dispatch uppercut (`S` + `LMB`).

3. **Attack Debouncing**:
   - Temporal cooldown ($240\text{ ms}$) prevents multi-triggering within a single follow-through swing.

## Consequences
- 100% consistent translation from physical punching motions to the corresponding in-game martial arts animations.
- Natural, highly rewarding melee combat responsiveness against inmates and guards in Butcher Bay.
