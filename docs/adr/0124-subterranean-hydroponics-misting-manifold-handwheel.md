# ADR 0124: Subterranean Hydroponics Misting Manifold Handwheel Siphon

## Status
Accepted

## Context
The subterranean botanical sectors rely on pressurized nutrient mist manifolds to cultivate bio-luminescent flora. Blocked atomizing nozzles cause toxic acidic vapor buildup. Riddick can manually siphon and purge the misting lines by opening high-pressure manifold handwheels to vent corrosive vapors onto patrolling guards or clear ventilation routes.

## Decision
1. **Manifold Handwheel Contact Zone**:
   - Controller positioned at horizontal siphon wheel station ($Z \in [-0.42, -0.15]\text{ m}, Y \in [-0.22, 0.16]\text{ m}$).
   - Grip held firmly ($Grip > 0.55$).
2. **Horizontal Tangential Revolution Tracking**:
   - Tracks rotational revolutions in the horizontal plane ($|\vec{V}_{xz}| > 0.48\text{ m/s}$).
   - Accumulates 3 full circular cycles ($\ge 1080^\circ$).
3. **Brass Packing Gland Friction & Aerosol Jet Haptics**:
   - Each quarter revolution triggers a gritty brass thread friction detent ($120\text{ Hz}$, $0.55$, $35\text{ ms}$).
   - On completion, emits a violent pressurized hydraulic purge pulse ($190\text{ Hz}$, $0.80$, $220\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches mist manifold siphon purge command (`E` + `Q` + `Enter`) with a $1500\text{ ms}$ lockout.

## Consequences
- Enriches subterranean botanical and environmental interaction options alongside specimen harvesting (ADR 0093) and nutrient plunger priming (ADR 0105).
- Provides tactical hazard manipulation to stagger adversaries or bypass contaminated chambers.
