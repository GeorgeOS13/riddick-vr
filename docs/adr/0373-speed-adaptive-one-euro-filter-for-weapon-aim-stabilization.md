# ADR 0373: Speed-Adaptive One-Euro Filter for One-Handed Weapon Aim Stabilization

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*, weapon aiming when holding firearms with two hands is stabilized via two-handed spatial smoothing (`s_twoHandedStabilizationActive`). However, when aiming one-handed (such as single-handed pistol/tranq gun firing or off-hand free movement), the raw OpenXR controller aim pose was passed without temporal filtering.

Natural human physiological hand tremor (typically 3–8 Hz oscillations with 1–3 mm amplitude) causes noticeable reticle and iron sight jitter in VR, particularly at longer engagement ranges. While aggressive static low-pass filtering can eliminate jitter, it introduces unacceptable latency during fast target acquisition or reflex flicks.

## Decision
Implement a speed-adaptive One-Euro low-pass filter for one-handed weapon aiming in `src/hook/camera_hook.cpp`:

1. **Velocity-Adaptive Filtering**:
   - Compute instantaneous controller linear speed:
     $$v = \sqrt{v_x^2 + v_y^2 + v_z^2}$$
   - When stationary ($v \le 0.10\text{ m/s}$), such as holding iron sights on target, apply high damping ($\alpha \approx 0.22$) to filter out physiological hand micro-tremor.
   - When transitioning to fast target sweeps ($0.10 < v < 0.45\text{ m/s}$), smoothly scale $\alpha$ up to $0.90$.
   - At high velocities ($v \ge 0.45\text{ m/s}$), tracking runs near raw sensor throughput ($\alpha \ge 0.90$) for zero noticeable latency.

2. **Quaternion SLERP & Normalization**:
   - Spherical linear interpolation and shortest-path sign correction ($\mathbf{q}_1 \cdot \mathbf{q}_2 \ge 0$) prevent orientation flipping artifacts during aiming sweeps.
   - Output quaternions are normalized every frame to eliminate floating-point drift.

## Consequences
- Eliminates weapon reticle jitter and micro-shaking during stationary aiming without introducing lag during rapid gunfights.
- Consistent, stable sight alignment for both one-handed and two-handed firearm handling.
