# ADR 0381: Ladder Rung Spatial Forward-Cone Filtering & Holster Decoupling

## Status
Accepted

## Context
In `src/hook/camera_hook.cpp`, initiating ladder climbing evaluated controller height and grip squeeze:
```cpp
if (rightLocal.y > 0.15f && rightController.grip > 0.5f) {
    s_rightHoldingRung = true;
    SendKeyTap('E');
}
```
Concurrently, reaching over the right shoulder to draw or stow long firearms (Slot 1, shotguns and assault rifles) evaluated:
```cpp
bool rightOverShoulder = (rightLocal.y > 0.15f && rightLocal.z > -0.05f && rightController.grip > 0.50f);
```
Because the ladder grab logic lacked a forward depth ($Z$) requirement, reaching over the shoulder behind the ear ($Z > -0.05$) simultaneously triggered BOTH the over-shoulder weapon draw (`SendKeyTap('1')`) AND the ladder rung grab (`SendKeyTap('E')`). In-game, this caused Riddick to interact with doors/consoles or drop weapons whenever attempting to holster or draw long-arms.

## Decision
Enforce spatial forward-cone depth filtering for ladder rung grabbing in `src/hook/camera_hook.cpp`:

1. **Forward Spatial Corridor**:
   - Physical ladder rungs are always positioned in front of the climber's chest and face.
   - Require controller position along $Z$ to be strictly within the forward corridor:
     $$Z \in [-0.55, -0.16]\text{ m}$$
   - Any hand position in line with the head or behind the ear plane ($Z > -0.16\text{ m}$) is rejected from ladder grabbing.

2. **Decoupled Holstering**:
   - Over-shoulder holstering ($Z > -0.05\text{ m}$) now operates with 100% isolation from ladder climbing, with zero key conflict.

## Consequences
- Clean separation between over-shoulder inventory interactions and ladder climbing.
- Prevents spurious interact key dispatches during combat weapon switches.
