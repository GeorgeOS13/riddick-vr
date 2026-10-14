# ADR 0070: Automated Turret Manual Traversal Handwheel Slew

## Status
Accepted

## Context
In *Assault on Dark Athena* and *Escape from Butcher Bay*, Riddick encounters manned defensive gun emplacements, override consoles, and dual-grip automated turret bastions (e.g., Mining Core perimeter guns and Dark Athena cargo bay turrets). In desktop play, these were operated via conventional mouse look or arrow keys.

In VR, operating heavy stationary artillery and mechanical turrets feels artificial when steered with simple 1:1 single-hand motion controller pointing. Heavy industrial turrets utilize dual spade grips or traverse handwheels with mechanical gearing, inertia, and pneumatic hydraulic dampening.

## Decision
We implement physical dual-grip turret traversal slew:
1. **Turret Console Dual-Grip Engagement:**
   - Both hands extended forward into the turret control station console ($Z_{local} \in [-0.42, -0.16]\text{ m}, Y_{local} \in [-0.30, +0.10]\text{ m}$).
   - Both controllers clutched firmly ($Grip_{left} > 0.55 \land Grip_{right} > 0.55$).
   - Horizontal hand separation within console spade grip dimensions ($|X_{left} - X_{right}| \in [0.25, 0.48]\text{ m}$).
2. **Turret Slew Steering & Inertial Lag:**
   - Turret azimuth (yaw) is steered by the differential forward/back offset of the dual grips ($\Delta Z = Z_{left} - Z_{right}$):
     - $\Delta Z > 0.08\text{ m} \implies$ traverse right (`D` / Right Arrow).
     - $\Delta Z < -0.08\text{ m} \implies$ traverse left (`A` / Left Arrow).
   - Turret elevation (pitch) is steered by collective vertical displacement ($\Delta Y = \frac{Y_{left} + Y_{right}}{2}$):
     - $\Delta Y > 0.12\text{ m} \implies$ elevate up (`Up Arrow`).
     - $\Delta Y < -0.12\text{ m} \implies$ depress down (`Down Arrow`).
3. **Heavy Gear Train Vibration:**
   - During traversal motion, emits synchronized low-frequency hydraulic gear tooth rumble across both controllers ($0.45$ amplitude at $55\text{ Hz}$ every $160\text{ ms}$).
   - Squeezing right trigger while gripped fires heavy rotary ammunition (`LMB` with dual $0.70$ cyclic recoil pulses).

## Consequences
- Transforms static gun emplacements into immersive physical combat platforms.
- Clear deadbands ($\pm 8\text{ cm}$ yaw, $\pm 12\text{ cm}$ pitch) ensure steady aiming without unwanted drift.
