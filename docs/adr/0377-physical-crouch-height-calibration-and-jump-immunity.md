# ADR 0377: Physical Crouch Height Calibration & Jump Immunity

## Status
Accepted

## Context
In `src/hook/camera_hook.cpp`, the standing eye height `s_standingEyeHeight` was calibrated dynamically:
```cpp
if (!s_isPhysicallyCrouched && currentY > s_standingEyeHeight) {
    s_standingEyeHeight = currentY;
}
```
This naive peak tracking contained a critical gameplay-breaking flaw:
When the player jumped (either physically or via the in-game jump input), the instantaneous vertical head position $Y$ surged $+20\text{ cm}$ to $+50\text{ cm}$ above normal eye level. The logic immediately latched this jump apex as the player's new "standing height".

Upon landing back on the floor, the player's true standing height was now $30\text{ cm}$ to $40\text{ cm}$ below the newly latched `s_standingEyeHeight`. The crouch detection logic (`heightDrop > 0.28f`) immediately engaged permanent crouching (`SendKey('C', true)`). The player became permanently stuck in crouch mode until manual recentering was executed.

## Decision
Refactor standing eye height tracking in `src/hook/camera_hook.cpp` to introduce jump immunity, vertical velocity gating, and rate-limited postural adaptation:

1. **Vertical Velocity Gating**:
   - Upward adaptation of `s_standingEyeHeight` is completely disabled whenever the HMD has significant vertical linear velocity ($|v_y| \ge 0.12\text{ m/s}$) or when an airborne/jump state is active.
   - This ensures jump trajectories, vaulting motions, and rapid vertical head bob cannot contaminate standing baseline calibration.

2. **Rate-Limited Postural Smoothing**:
   - Rather than instantaneous step latching, subtle posture changes (such as standing up straighter or adjusting headset straps) adapt via a gradual exponential low-pass filter:
     $$h_{\text{standing}} \leftarrow h_{\text{standing}} + 0.002 \times (Y_{\text{current}} - h_{\text{standing}})$$
   - This prevents transient head movements from destabilizing the calibrated floor plane.

3. **Instant Recenter Bypass**:
   - Intentional recenter commands (`vrState.shouldRecenter` or Right Stick Click) continue to immediately reset `s_standingEyeHeight` to current HMD height when stationary.

## Consequences
- Completely eliminates permanent crouch lock bugs following jumps or vertical movements.
- Provides robust, zero-drift floor calibration across hours of active room-scale gameplay.
