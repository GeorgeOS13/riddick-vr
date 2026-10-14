# ADR 0108: Subterranean Mining Rail Turntable Push Lever

## Status
Accepted

## Context
In the deep subterranean mining sector of Butcher Bay, heavy ore transport carts navigate complex multi-branch track junctions via massive circular rail turntables. On conventional flat screens, these were turned using static button interactions. In VR, physical immersion requires grabbing the peripheral push bar / capstan handle and rotating the heavy cast-iron platform manually.

## Decision
1. **Turntable Lever Grasp Zone**:
   - Controller positioned at turntable circumference ($Z_{local} \in [-0.45, -0.16]\text{ m}, Y_{local} \in [-0.40, -0.05]\text{ m}$).
   - Grip clenched firmly ($Grip > 0.55$).
2. **Circular Tangential Haul Gesture**:
   - Planar horizontal hand velocity ($|V_x| > 0.50\text{ m/s} \lor |V_z| > 0.50\text{ m/s}$).
   - Requires accumulating 3 successive rotational strides to complete a track realignment quadrant.
3. **Mechanical Rail Turntable Haptics**:
   - Continuous $55\text{ Hz}$ ($0.65$) heavy ball-bearing grinding rumble during push strokes.
   - Concluding $80\text{ Hz}$ ($0.90$) concussive mechanical detent lock when the turntable snaps into alignment.
4. **Engine Input Injection**:
   - Dispatches rail turntable realignment sequence (`E` + `Space` + `Tab`) with an $1800\text{ ms}$ latch deadband.

## Consequences
- Transforms mining rail puzzle navigation into an active physical exertion mechanic.
- Integrates seamlessly with mining cart throttle and brake controls (ADR 0048).
