# ADR 0167: Dark Athena EVA Solar Array Debris Deflection Grid Pitch Tilt Pivot

## Context
During orbital transit through micro-meteoroid swarms and debris fields around planetary rings, the *Dark Athena*'s solar array articulation pivots are shielded by heavy reinforced deflection grids. These grids can be angled relative to the ship's flight vector to deflect high-velocity orbital particulate away from sensitive solar panel joint mechanisms. Riddick can manually grasp the tilt yoke with both hands and pitch the grid into its protective alignment.

## Decision
Implement physical EVA debris deflection grid tilt manipulation via two-handed 6DOF tracking:
1. **Two-Handed Yoke Grasp**:
   - Both hands positioned at the tilt yoke handles:
     - $dist(L, R) \in [0.20, 0.40]\text{ m}$.
     - $Z_{controller} \in [-0.42, -0.15]\text{ m}$.
     - $Y_{controller} \in [0.10, 0.40]\text{ m}$.
     - $Grip_{left} > 0.60 \land Grip_{right} > 0.60$.
2. **Pitch-Plane Angular Tilt**:
   - Executing a coordinated pitch sweep in the sagittal $YZ$ plane:
     - $|\vec{V}_{yz}| = \sqrt{V_y^2 + V_z^2} > 0.60\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `E` (Interact) + `Space` (Actuator clamp) + `Tab` (Orientation lock) for $160\text{ ms}$.
4. **Haptic Feedback**:
   - Dual-hand $140\text{ Hz}$ ($0.65$) heavy gimbal bearing friction during motion.
   - $270\text{ Hz}$ ($0.95$) solid mechanical quadrant detent lock when set.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Adds an authentic heavy mechanical interaction to orbital spacewalk sections.
- Employs coordinated two-handed angular kinematics.
