# ADR 0079: Atmospheric Pressure Equalization Hatch Bleed Petcock Valve

## Status
Accepted

## Context
In *Assault on Dark Athena* and *Escape from Butcher Bay*, high-pressure vacuum bulkheads, cryo-containment chambers, and deep-level airlocks feature atmospheric pressure differentials between compartments. In the original flat-screen game, doors simply triggered an automatic equalization sequence or general interaction key press.

In VR, physical immersion is greatly amplified when players must manually locate and twist the equalization needle petcock valve to bleed pressure across compartments before heavy hydraulic door clamps can be released.

## Decision
We implement physical pressure equalization petcock valve twisting:
1. **Spatial Engagement:**
   - Dominant controller positioned at the door-frame bypass valve hub ($Z_{local} \in [-0.40, -0.15]\text{ m}, Y_{local} \in [-0.10, +0.20]\text{ m}$).
   - Controller grip clutched ($Grip > 0.50$).
2. **Wrist Twist Velocity:**
   - Rapid rotational or horizontal wrist flick ($|V_x| > 0.45\text{ m/s} \lor |V_y| > 0.45\text{ m/s}$).
   - Cooldown buffer: $900\text{ ms}$.
3. **Engine Action Dispatch & High-Frequency Pneumatic Haptics:**
   - Dispatches interaction key (`E`) to initiate compartment atmospheric bleed.
   - Generates high-frequency piercing pneumatic hiss and needle valve rattle micro-haptics ($0.55$ amplitude at $220\text{ Hz}$ for $200\text{ ms}$) simulating super-atmospheric equalization.

## Consequences
- Heightens the realism and tension of pressure-critical airlock traversal.
- Velocity filtering prevents false triggers when resting hands near door frames.
