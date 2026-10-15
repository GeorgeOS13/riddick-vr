# ADR 0394: Physical Reload Gesture Ergonomics & Mag-Well Insertion Gating

## Context
Previously, physical reload detection in `CameraHook::Update` suffered from two severe ergonomic issues:
1. Reaching to the waist pouch for ammunition checked `(rightLocal.y < -0.38f && vrState.rightController.grip > 0.50f)`. Because the dominant hand holds the weapon, lowering or resting the arm down by the hip while holding the controller handle continuously triggered accidental reloads (`SendKeyTap('R')`).
2. Two-handed weapon stabilization (`twoHandDist < 0.28m`) and off-hand receiver reload (`reloadHandDist < 0.20m`) overlapped. When gripping a firearm foregrip to steady aim, the player frequently triggered unwanted reloads, locking out aim steering.

## Decision
1. Restrict waist pouch ammunition retrieval strictly to the off-hand (`leftLocal.y < -0.38m`, $X \in [-0.35, -0.05]\text{ m}$, `leftController.grip > 0.50f`), eliminating accidental reloads when resting the dominant arm.
2. Require an active upward magazine seating insertion stroke ($V_y > 0.35\text{ m/s}$) when bringing the off-hand to the weapon receiver ($< 18\text{ cm}$), distinguishing intentional magazine insertion from static two-handed aiming.
3. Automatically suppress off-hand reload detection whenever two-handed firearm stabilization (`s_twoHandedStabilizationActive`) is engaged.

## Consequences
- Completely eliminates accidental reloads when walking with lowered arms or holding a two-handed weapon grip.
- Physical magazine seating and tactical pouch reloads feel deliberate, tactile, and natural.
