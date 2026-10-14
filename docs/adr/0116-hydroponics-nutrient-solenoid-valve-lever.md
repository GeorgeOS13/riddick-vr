# ADR 0116: Subterranean Hydroponics Nutrient Dosing Solenoid Lever

## Status
Accepted

## Context
In the subterranean hydroponic facility beneath Butcher Bay, agricultural drip manifolds deliver high-potency mineral feeds to bio-luminescent plants used for contraband brewing and healing salves. Players must manually prime the delivery manifold by throwing the heavy wall-mounted solenoid valve lever upward.

## Decision
1. **Solenoid Valve Lever Zone**:
   - Controller positioned at wall valve manifold ($Z_{local} \in [-0.42, -0.15]\text{ m}, Y_{local} \in [-0.25, 0.15]\text{ m}$).
   - Grip held firmly ($Grip > 0.50$).
2. **Upward Solenoid Prime Haul**:
   - Upward lever throw gesture: $V_y > 0.65\text{ m/s}$.
3. **Electromagnetic Solenoid Haptics**:
   - High-force electromagnetic armature clack ($110\text{ Hz}$, $0.80$, $75\text{ ms}$) followed by fluid surging through pipe conduits ($70\text{ Hz}$, $0.45$, $150\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches manifold dosing signal (`E` + `Space` + `Q`) with a $1500\text{ ms}$ reset lockout.

## Consequences
- Adds an authentic, tactile industrial pipe mechanic to the subterranean biome.
- Seamlessly pairs with plant specimen harvesting (ADR 0093) and pressure plunger clearing (ADR 0105).
