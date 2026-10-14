# ADR 0114: Dark Athena EVA Solar Array Manual Hand Crank

## Status
Accepted

## Context
During power failures aboard the Dark Athena, exterior photovoltaic collector arrays lose automated stellar tracking. To restore auxiliary power to critical life support systems, Riddick must traverse the exterior truss in zero-gravity and manually turn the geared worm-drive spindle crank to align the panels with the solar vector.

## Decision
1. **Solar Array Spindle Crank Zone**:
   - Both hands positioned at exterior truss hub ($Z_{local} \in [-0.42, -0.16]\text{ m}, Y_{local} \in [-0.15, 0.25]\text{ m}$).
   - Both grips clenched ($Grip_{left} > 0.50 \land Grip_{right} > 0.50$).
2. **Circular Geared Revolution Gesture**:
   - Tangential speed in the front coronal plane: $\sqrt{V_x^2 + V_y^2} > 0.50\text{ m/s}$.
   - Requires 3 full rotational revolutions to lock the array tracking mechanism.
3. **Worm-Drive Planetary Gear Haptics**:
   - Rhythmic planetary gear teeth clicks ($150\text{ Hz}$, $0.50$, $30\text{ ms}$) on each revolution.
   - Deep electromagnetic solar collector latch hum ($90\text{ Hz}$, $0.85$, $150\text{ ms}$) when alignment is achieved.
4. **Engine Input Injection**:
   - Dispatches auxiliary solar array alignment signal (`E` + `Enter` + `W`) with a $2000\text{ ms}$ lockout.

## Consequences
- Deepens exterior spacewalk environmental interaction variety beyond airlocks.
- Fully integrated with EVA magnetic boot anchoring dynamics (ADR 0040).
