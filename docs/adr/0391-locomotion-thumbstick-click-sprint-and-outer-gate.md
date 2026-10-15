# ADR 0391: Locomotion Thumbstick Click Sprint & Outer Deflection Gating

## Context
In first-person gameplay for *The Chronicles of Riddick: Assault on Dark Athena*, sprinting requires holding `VK_SHIFT` (Left Shift).
Previously, the VR controller mapping only supported discrete forward walking (`W`), with no mapped trigger or stick button for sprinting. Consequently, the player was permanently restricted to base walking speed during VR room-scale exploration and evasive combat maneuvers.

## Decision
1. Implement VR FPS standard thumbstick click sprint:
   - When moving forward (`wantW`), clicking the left locomotion thumbstick (`RIDDICK_VR_BTN_STICK_LEFT`) toggles/engages sprint (`SendKey(VK_SHIFT, true)`), accompanied by a crisp 25ms tactile click confirmation (`vrState.leftHapticPulse = 0.25f`).
   - Pushing the left thumbstick to its extreme forward limit (`stickLY > 0.92f`) automatically engages sprint for players who prefer tilt-threshold running over continuous stick clicking.
2. Automatically disengage sprint when the player releases forward movement, enters a crouch (`s_isPhysicallyCrouched`), prones, or mounts a ladder.

## Consequences
- Restores full evasive sprint mobility and tactical repositioning in VR.
- Aligns control scheme with industry-standard VR FPS locomotion ergonomics.
