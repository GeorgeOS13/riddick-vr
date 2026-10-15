# ADR 0379: Two-Handed Weapon Foregrip Distance Hysteresis & Detach Filtering

## Status
Accepted

## Context
In *The Chronicles of Riddick* VR mod, two-handed firearm stabilization is engaged when bringing the off-hand controller close to the dominant weapon hand (`src/hook/camera_hook.cpp`). Previously, the logic tested a single distance threshold:
```cpp
bool wantTwoHanded = (twoHandDist < 0.25f) && (leftController.grip > 0.45f);
```
Because the attach threshold and release threshold were identical ($25\text{ cm}$), whenever the player held long firearms (rifles, shotguns) near the $24\text{ cm}$ to $26\text{ cm}$ boundary, natural tracking micro-jitter caused the two-handed stabilization state to flutter on and off dozens of times per second. This produced continuous grip vibration buzzing, state oscillation, and log spam.

## Decision
Implement dual-threshold distance and grip hysteresis for two-handed firearm stabilization in `src/hook/camera_hook.cpp`:

1. **Distance Hysteresis**:
   - **Attach Distance**: Two-handed stabilization engages when the off-hand enters within $28\text{ cm}$ ($d_{\text{attach}} \le 0.28\text{ m}$).
   - **Detach Distance**: Once engaged, the stabilization holds until the hands separate beyond $38\text{ cm}$ ($d_{\text{detach}} \ge 0.38\text{ m}$).
   - This provides a generous $10\text{ cm}$ stability buffer, completely eliminating boundary flutter.

2. **Grip Force Hysteresis**:
   - Engaging foregrip requires positive squeeze ($\text{grip} > 0.50$).
   - Releasing requires relaxing the grip below $\text{grip} < 0.30$.

3. **Haptic Confirmation**:
   - A crisp $0.25$ haptic click confirms foregrip attachment; disengagement is silent.

## Consequences
- Solid, jitter-free two-handed firearm handling across all weapon classes.
- Completely eliminates rapid state fluttering and excessive haptic buzzing.
