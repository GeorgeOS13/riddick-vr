# ADR 0372: Radial Locomotion Deadzone & Diagonal Movement Angle Normalization

## Status
Accepted

## Context
Locomotion in `src/hook/camera_hook.cpp` previously mapped left thumbstick deflections directly to keyboard WASD inputs using independent axial thresholds (`thumbstickY > 0.25f` for W, `thumbstickX > 0.25f` for D).

Axial deadzones introduce two significant problems in virtual reality locomotion:
1. **Accidental Diagonal Strafing**: When attempting to walk purely straight forward, the human thumb naturally fluctuates by $\pm 5^\circ$ to $15^\circ$ off the exact vertical axis. With an independent X threshold of 0.25, pushing forward-right triggers both 'W' and 'D', causing the player to veer sideways unintentionally.
2. **Cardinal Axis Stickiness & Uneven Deadzones**: The effective deadzone is square rather than circular. At diagonal stick deflections ($X=0.25, Y=0.25$), the stick magnitude is $\sqrt{0.25^2 + 0.25^2} \approx 0.354$, requiring more thumb displacement diagonally to break out of the deadzone than along the cardinal axes.

## Decision
Implement a true circular/radial deadzone with directional sector mapping and forward-priority deadband in `src/hook/camera_hook.cpp`:

1. **Radial Magnitude Calculation**:
   $$\text{moveMag} = \sqrt{\text{stickX}^2 + \text{stickY}^2}$$
   If $\text{moveMag} < \text{RADIAL\_DEADZONE} \ (0.22)$, all locomotion inputs are zeroed, preventing micro-drift.

2. **Normalized Direction Vector**:
   $$\hat{d} = (\text{dirX}, \text{dirY}) = \left(\frac{\text{stickX}}{\text{moveMag}}, \frac{\text{stickY}}{\text{moveMag}}\right)$$

3. **Forward-Priority Angular Deadband**:
   - Walking straight forward or backward is given an angular deadband of approximately $\pm 25^\circ$ ($\sin 25^\circ \approx 0.42$):
     - Forward ('W'): $\text{dirY} > 0.35$
     - Backward ('S'): $\text{dirY} < -0.35$
     - Right Strafe ('D'): Engaged only if $\text{dirX} > 0.42$
     - Left Strafe ('A'): Engaged only if $\text{dirX} < -0.42$
   - This ensures that slight sideways variance while pushing forward does not trigger accidental strafing, while intentional diagonal movement (stick pushed into diagonal quadrants, $>25^\circ$) smoothly activates both W+D or W+A.

4. **Climbing & Gesture Override**:
   - Ladder climbing gestures (`s_ladderClimbUp`, `s_ladderClimbDown`) continue to take precedence and drive W/S seamlessly.

## Consequences
- Clean, intuitive locomotion with zero unintentional sideways drift during forward walking.
- Consistent circular deadzone response across all thumbstick deflection angles.
- Immediate responsiveness when deliberately navigating diagonally through corridors.
