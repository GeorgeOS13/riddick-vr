# ADR 0382: Off-Hand Throwing Trajectory Forward Vector Gating

## Status
Accepted

## Context
In *The Chronicles of Riddick*, players can grab objects, severed heads, and explosive canisters with the off-hand and throw them at enemies. In `src/hook/camera_hook.cpp`, releasing Left Grip evaluated scalar linear speed:
```cpp
if (lSpeed > 1.2f) {
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0); // Dispatch throw
}
```
Because `lSpeed` is an omnidirectional scalar speed, releasing the grip while retracting the arm toward the chest, raising the hand, or moving sideways at speeds $> 1.2\text{ m/s}$ was misinterpreted as an intentional forward throw. This caused carried objects to be forcefully hurled against nearby walls or into the floor when the user simply wanted to pull the object closer or drop it.

## Decision
Incorporate directional trajectory gating on the off-hand velocity vector in `src/hook/camera_hook.cpp`:

1. **Forward Vector Alignment**:
   - Require negative velocity along the view forward axis ($Z$):
     $$v_z < -0.65\text{ m/s}$$
   - And total speed exceeding the threshold:
     $$v_{\text{total}} > 1.10\text{ m/s}$$

2. **Gentle Drop Default**:
   - If released without forward momentum ($v_z \ge -0.65\text{ m/s}$), the object is released gently at the player's feet (`SendKey('E', false)`), allowing quiet stealth placement.

## Consequences
- Physical throwing requires an unambiguous forward tossing or pushing motion.
- Eliminates accidental loud clatter and unwanted explosive canister detonations.
