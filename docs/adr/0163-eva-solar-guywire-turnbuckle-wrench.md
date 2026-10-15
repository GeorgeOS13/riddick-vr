# ADR 0163: Dark Athena EVA Solar Collector Photovoltaic Array Tension Guy-Wire Turnbuckle Wrench

## Context
Exterior solar collector panels on the *Dark Athena* are stabilized against thruster vibration and orbital micro-drag by steel guy-wire rigging. Over time, thermal cycling between orbital sunlight and eclipse shadows stretches these cables, causing angular misalignment and panel oscillation. To restore structural rigidity, Riddick can manually grasp and twist the turnbuckle sleeve located at the cable anchor terminals.

## Decision
Implement physical EVA solar array guy-wire turnbuckle tensioning via 6DOF wrist rotation and spatial interaction:
1. **Turnbuckle Sleeve Contact**:
   - Dominant hand positioned at guy-wire anchor junction:
     - $Z_{controller} \in [-0.42, -0.15]\text{ m}$.
     - $Y_{controller} \in [-0.20, 0.20]\text{ m}$.
     - $Grip_{dominant} > 0.60$.
2. **Torque Twist Motion**:
   - Applying a rotational wrist twist or lateral wrench stroke:
     - $|\Delta\text{roll}| > 1.2\text{ rad} \lor |V_{x, dominant}| > 0.65\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `E` (Interact) + `W` (Forward throttle) + `Space` (Actuator tension) for $140\text{ ms}$.
4. **Haptic Feedback**:
   - $150\text{ Hz}$ ($0.55$) high-tensile wire friction hum during rotation.
   - $290\text{ Hz}$ ($0.95$) mechanical turnbuckle locking detent snap when fully tensioned.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Broadens the suite of mechanical spacewalk EVA operations.
- Enhances player immersion when traversing exterior ship geometry in zero-g.
