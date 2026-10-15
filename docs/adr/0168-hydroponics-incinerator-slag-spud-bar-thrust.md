# ADR 0168: Subterranean Hydroponics Bio-Waste Incinerator Slag Poking Spud Bar Thrust

## Context
In the Subterranean Hydroponics bio-waste incinerator, continuous combustion of organic waste forms hard, glassy clinker slag across the grate bars and burner nozzles. This slag impedes airflow and chokes the combustion cycle unless manually shattered and cleared. Riddick can grasp a heavy steel spud bar with both hands and drive it through the inspection port to break up slag crusts.

## Decision
Implement physical two-handed slag breaking spud bar thrust via 6DOF motion tracking:
1. **Two-Handed Bar Grasp**:
   - Hands spaced along the spud bar handle:
     - $dist(L, R) < 0.28\text{ m}$.
     - $Z_{controller} \in [-0.42, -0.16]\text{ m}$.
     - $Y_{controller} \in [-0.25, 0.15]\text{ m}$.
     - $Grip_{left} > 0.60 \land Grip_{right} > 0.60$.
2. **Forward Linear Strike**:
   - Plunging the spud bar forward with high velocity:
     - $V_{z, right} < -1.15\text{ m/s} \lor V_{z, left} < -1.15\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `E` (Interact) + `LMB` (Kinetic strike) + `Q` (Tuyere clear) for $180\text{ ms}$.
4. **Haptic Feedback**:
   - Dual-hand $260\text{ Hz}$ ($0.98$) vitrified slag fracture percussion shockwave.
   - Secondary $90\text{ Hz}$ ($0.60$) resonant iron bar reverberation.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Extends tactile environmental clearing interactions with a satisfying heavy physical thrust.
- Enhances facility maintenance immersion in dark industrial zones.
