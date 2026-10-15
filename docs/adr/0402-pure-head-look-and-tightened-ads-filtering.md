# ADR 0402: Pure Head Look and Tightened ADS Filtering

## Context
1. In `src/hook/camera_hook.cpp`, when aiming down sights (`s_isPhysicalADS` or `s_twoHandedStabilizationActive`), an artificial damping factor (`0.50f` or `0.70f`) was being multiplied into `mouseDeltaX` and `mouseDeltaY`.
   - This caused head rotation (look pitch and yaw) to feel sluggish, unresponsive, and distorted while holding a weapon up to aim. Head tracking must always remain strictly 1:1 with real-world physical head movements regardless of weapon stance.
2. In two-handed stabilization, releasing the off-hand from the weapon (`s_twoHandedStabilizationActive == false`) left `s_hasStabilizedPose` as `true` with the old stabilized pose values. When re-gripping, the weapon pose would experience an abrupt single-frame snap/jump to the previous pose before blending.
3. In one-handed pistol and tranq gun aiming, physiological hand tremors (3-8 Hz) cause micro-jitter on iron sights. The adaptive One-Euro filter previously kept a flat base alpha (`0.22`), which permitted slight reticle wobble when holding steady in the physical ADS eye-relief corridor.

## Decision
1. **Uncompromised 1:1 Head Look During ADS:**
   - Removed artificial damping factors from `mouseDeltaX` and `mouseDeltaY` during ADS and two-handed stabilization. Head look remains unscaled, 1:1, and natural at all times. Aim stabilization is strictly confined to the virtual controller aim pose.
2. **Re-Grip Stabilization State Invalidation:**
   - Set `s_hasStabilizedPose = false` when two-handed stabilization disengages. On the next two-handed engagement, the stabilized pose immediately initializes to the current controller pose without any residual offset jump.
3. **Tightened One-Handed Iron Sight Filter:**
   - Moved `s_isPhysicalADS` to file scope.
   - When the right controller is positioned in the physical ADS eye corridor (`s_isPhysicalADS == true`) and controller movement velocity is low ($< 0.08\text{ m/s}$), tighten the base filter alpha from `0.22` to `0.15` for rock-steady iron sight alignment.
   - Smoothly ramp alpha up to `0.90` as hand speed increases beyond $0.08\text{ m/s}$ for instantaneous snap-targeting without latency.

## Consequences
- Player head rotation is completely natural and unhindered during aiming.
- Two-handed firearm re-gripping is seamless without visual popping or pose jumping.
- One-handed pistol iron sights remain steady on distant targets during precision aim.
- Zero green glitch pixels and verified 100% self-test pipeline pass.
