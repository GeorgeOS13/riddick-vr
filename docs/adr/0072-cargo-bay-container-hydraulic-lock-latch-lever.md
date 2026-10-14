# ADR 0072: Cargo Bay Container Hydraulic Lock Latch Lever

## Status
Accepted

## Context
In the cavernous cargo holds of the Dark Athena and the subterranean supply depots of Butcher Bay, massive freight containers and modular cargo pods house munitions, heavy weapons, and high-value contraband. These heavy industrial containers are sealed with vertical hydraulic dogging levers. In the original desktop game, unlocking container latches was performed by pressing the general interaction key (`E`).

In VR, opening heavy shipping containers should require players to physically grab the locking latch and apply upward leverage to break the hydraulic seal.

## Decision
We implement physical cargo container hydraulic lock lever manipulation:
1. **Spatial Engagement Zone:**
   - Dominant or off-hand controller positioned in front of container door ($Z_{local} \in [-0.42, -0.16]\text{ m}, Y_{local} \in [-0.25, +0.15]\text{ m}$).
   - Controller grip engaged ($Grip > 0.50$) securing the latch handle.
2. **Upward / Outward Lever Throw Velocity:**
   - Upward or outward lever arm motion exceeding $+0.70\text{ m/s}$ ($V_y > +0.70\text{ m/s} \lor |V_x| > +0.70\text{ m/s}$).
   - Re-trigger buffer: $900\text{ ms}$.
3. **Engine Action Dispatch & Hydraulic Decompression Feedback:**
   - Dispatches native interaction key (`E`) to unlock and swing container doors.
   - Generates heavy pneumatic decompression and metal latch snap haptics ($0.85$ amplitude at $100\text{ Hz}$ for $160\text{ ms}$) simulating the sudden release of pressurized door seals.

## Consequences
- Transforms container exploration into an authentic, visceral physical interaction.
- Thresholds prevent false-triggers during general standing controller movement.
