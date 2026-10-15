# ADR 0152: Subterranean Hydroponics Bio-Waste Incinerator Blast Door Dual Lever Pull

## Status
Accepted

## Context
In the bio-filtration and methane disposal sectors, dangerous incinerator vaults house burning biomass. Safety protocols require that heavy pneumatic blast doors can only be manually released by pulling two mechanically linked wall levers simultaneously. This prevents accidental decompression or backfire while ensuring Riddick can vent toxic flames against pursuing riot guards.

In VR, dual-lever physical coordination conveys the sheer physical mass and industrial power of the prison ship's subterranean infrastructure.

## Decision
We implement **Subterranean Hydroponics Bio-Waste Incinerator Blast Door Dual Lever Pull**:
1. **Dual Console Lever Corridor**:
   - Hands separated across console width ($dist(Left, Right) \in [0.22, 0.45]\text{ m}$, $Z \in [-0.42, -0.16]\text{ m}$, $Y \in [0.05, 0.35]\text{ m}$).
   - Both controller grips clenched ($leftGrip > 0.60 \land rightGrip > 0.60$) on the lever handles.
2. **Simultaneous Downward Pull**:
   - Hauling downward simultaneously ($V_{y, left} < -0.80\text{ m/s} \land V_{y, right} < -0.80\text{ m/s}$) trips the counterweighted mechanical latches.
3. **Input Dispatch**:
   - Dispatches `E` (interact) + `Enter` (incinerator blast door cycle) + `Q` (thermal vent purge).
4. **Haptic Feedback**:
   - Dual-hand $0.90$ ($110\text{ Hz}$) mechanical lever trip clack followed by an intense $50\text{ Hz}$ ($0.95$) low-frequency thermal combustion rumble.

## Consequences
- Authentic multi-limb industrial puzzle solving.
- Fully debounced with a $1500\text{ ms}$ cooldown.
