# ADR 0175: Dark Athena EVA Solar Collector Truss Articulation Lock Ring Spanner Wrench

## Context
Main solar array structural booms on the *Dark Athena* are fastened to orbital truss masts via heavy alloy lock rings with perimeter spanner lugs. During structural reconfiguration or emergency wing jettison, these threaded lock rings must be unseated using a high-leverage dual-handled spanner tool. Riddick must physically grip the spanner handles with both hands and haul downward with his body weight.

## Decision
Implement two-handed solar truss lock ring spanner torque wrenching via 6DOF tracking:
1. **Two-Handed Spanner Grasp**:
   - Both hands positioned at lock ring spanner grips:
     - $dist(L, R) < 0.25\text{ m}$.
     - $Z_{controller} \in [-0.42, -0.15]\text{ m}$.
     - $Y_{controller} \in [0.10, 0.40]\text{ m}$.
     - $Grip_{left} > 0.60 \land Grip_{right} > 0.60$.
2. **Coordinated Downward Torque Pull**:
   - Hauling downward simultaneously with high velocity:
     - $V_{y, left} < -0.90\text{ m/s} \land V_{y, right} < -0.90\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `E` (Interact) + `Space` (Actuator release) + `Enter` (Lock ring unclamp) for $160\text{ ms}$.
4. **Haptic Feedback**:
   - Dual-hand $150\text{ Hz}$ ($0.70$) notched gear unseating crack.
   - $290\text{ Hz}$ ($0.95$) mechanical ring release detent thud upon full unclamp.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Deepens mechanical realism during zero-g spacewalk exterior maintenance.
- Employs intuitive vertical two-handed leverage.
