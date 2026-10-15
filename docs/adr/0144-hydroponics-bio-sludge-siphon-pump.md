# ADR 0144: Subterranean Hydroponics Bio-Sludge Siphon Pump T-Handle Reciprocating Stroke

## Status
Accepted

## Context
In the lower drainage sump and hydroponic slurry recycling levels, thick biological waste and sediment clog runoff culverts, backing up foul sludge and locking pneumatic maintenance bulkheads. To drain the flooded sumps and unlock escape passages, Riddick must operate a manual double-acting T-handle lift pump.

In VR, this provides an engaging full-body, two-handed interaction requiring rhythmic downward reciprocating strokes against viscous hydraulic resistance.

## Decision
We implement **Subterranean Hydroponics Bio-Sludge Siphon Pump T-Handle Reciprocating Stroke**:
1. **Two-Handed T-Handle Corridor**:
   - Hands together at pump handle ($dist(Left, Right) < 0.26\text{ m}$, $Z \in [-0.38, -0.14]\text{ m}$, $Y \in [-0.35, 0.15]\text{ m}$).
   - Both controller grips clenched ($leftGrip > 0.55 \land rightGrip > 0.55$).
2. **Reciprocating Stroke Count**:
   - Executing 3 full downward pump strokes ($V_{y, left} < -0.70\text{ m/s} \land V_{y, right} < -0.70\text{ m/s}$) separated by minimum $240\text{ ms}$ reset intervals.
3. **Input Dispatch**:
   - Upon the 3rd completed stroke, dispatches `E` (interact) + `Space` (drain valve lock) + `W` (pump forward cycle).
4. **Haptic Feedback**:
   - Viscous hydraulic fluid resistance pulses ($70\text{ Hz}$, $0.65$) on each downstroke, with a resonant $120\text{ Hz}$ ($0.95$) pneumatic surge release upon clearing the line.

## Consequences
- Authentic industrial interaction for the subterranean infrastructure puzzles.
- Fully debounced with persistent stroke reset timing.
