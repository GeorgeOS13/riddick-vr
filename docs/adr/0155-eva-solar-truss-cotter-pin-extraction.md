# ADR 0155: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Cotter Pin Extraction

## Status
Accepted

## Context
When folding damaged or obstructing solar array photovoltaic collector wings on the exterior hull of the *Dark Athena*, or when salvaging high-tensile spring-steel pins for improvised tools and shiv blanks, Riddick must remove the locking cotter pin from the collector truss pivot collar.

In VR, physical immersion requires tactile extraction gestures: grasping the looped pin head and delivering a forceful rearward extraction stroke against retention spring detents.

## Decision
We implement **EVA Solar Collector Truss Articulation Lock Hinge Cotter Pin Extraction**:
1. **Pivot Collar Envelope**:
   - Dominant hand positioned at the hinge retention collar ($Z \in [-0.42, -0.15]\text{ m}$, $Y \in [0.10, 0.40]\text{ m}$).
   - Dominant grip held ($> 0.60$) clutching the pin head.
2. **Rearward Extraction Stroke**:
   - Delivering a sharp rearward extraction pull ($V_z > 0.85\text{ m/s}$) pulls the pin free from the bore.
3. **Input Dispatch**:
   - Dispatches `E` (interact) + `Space` (unclamp / hinge release) + `Tab` (salvage inventory).
4. **Haptic Feedback**:
   - $220\text{ Hz}$ ($0.65$) detent slide drag followed by a resonant $180\text{ Hz}$ ($0.95$, $100\text{ ms}$) mechanical release clunk.

## Consequences
- Complements ADR 0151 (insertion) to create a fully reversible, tactile physical maintenance cycle.
- Debounced with a $1500\text{ ms}$ refractory cooldown.
