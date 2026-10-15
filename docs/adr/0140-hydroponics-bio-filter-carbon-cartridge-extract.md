# ADR 0140: Subterranean Hydroponics Bio-Filter Carbon Mesh Cartridge Extraction

## Status
Accepted

## Context
In the subterranean hydroponics and atmosphere recirculation sectors of Butcher Bay and the Dark Athena, biological waste processing and algae scrubbers accumulate toxic sludge and saturated methane. Riddick must frequently clear obstructed intake manifolds by physically pulling fouled cylindrical carbon honeycomb cartridges from sealed wall channels to restore air flow or retrieve contraband stashed within the filter housing.

In VR, this provides a tactile two-handed extraction mechanic requiring coordinate pull forces and conveying the realistic sliding friction of rubber hermetic seals.

## Decision
We implement **Subterranean Hydroponics Bio-Filter Carbon Mesh Cartridge Extraction**:
1. **Two-Handed Grip Corridor**:
   - Hands in close proximity ($dist(Left, Right) < 0.28\text{ m}$) within the filter canal envelope ($Z \in [-0.38, -0.14]\text{ m}$, $Y \in [-0.20, 0.20]\text{ m}$).
   - Both controller grips clenched ($leftGrip > 0.55 \land rightGrip > 0.55$) onto the cartridge extraction handles.
2. **Rearward Slide Impulse**:
   - Simultaneous rearward horizontal haul ($V_{left, z} > 0.65\text{ m/s} \land V_{right, z} > 0.65\text{ m/s}$) slides the heavy carbon canister out of its pressurized retaining dogs.
3. **Input Dispatch**:
   - Dispatches `E` (interact) + `Q` (valve/manifold state toggle) + `Tab` (contraband/objective registration).
4. **Haptic Feedback**:
   - Dual-hand $0.85$ ($180\text{ Hz}$) mechanical sliding detent and latch release clunk.

## Consequences
- Seamlessly integrates with the subterranean bio-filtration puzzles and stealth ventilation routes.
- Fully debounced with a $1500\text{ ms}$ refractory cooldown.
