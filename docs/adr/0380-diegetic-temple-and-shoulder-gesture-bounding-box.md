# ADR 0380: Diegetic Temple & Shoulder Gesture Spatial Bounding Box Isolation

## Status
Accepted

## Context
In `src/hook/camera_hook.cpp`, gestures for toggling Riddick's Eyeshine night vision (Right Temple Tap, key `'Q'`) and Tactical Shoulder Flashlight (Left Shoulder Tap, key `'F'`) previously checked only horizontal and vertical offsets ($X$ and $Y$):
```cpp
bool rightAtTemple = (rightLocal.y > 0.08f && rightLocal.x > 0.12f && rightController.grip > 0.5f);
bool leftAtShoulder = (leftLocal.y > 0.06f && leftLocal.x < -0.12f && leftController.grip > 0.5f);
```
Because the depth axis ($Z$) was completely unbounded, any high-reach action in front of the body (such as climbing high ladder rungs, mantling ledges, or raising both fists into high guard) could accidentally satisfy the criteria and toggle Eyeshine or flashlight unexpectedly.

## Decision
Constrain both gestures within tight 3D anatomical bounding boxes centered on the player's head coordinate system in `src/hook/camera_hook.cpp`:

1. **Right Temple Bounding Box (Eyeshine)**:
   - $X \in [0.12, 0.28]\text{ m}$ (right lateral ear/temple plane)
   - $Y \in [0.06, 0.22]\text{ m}$ (eye/brow vertical level)
   - $Z \in [-0.15, 0.08]\text{ m}$ (along ear canal / temple axis)
   - Hands extended forward ($Z < -0.15\text{ m}$) or backward ($Z > 0.08\text{ m}$) are strictly excluded.

2. **Left Shoulder Bounding Box (Flashlight)**:
   - $X \in [-0.28, -0.12]\text{ m}$ (left clavicle/shoulder zone)
   - $Y \in [-0.10, 0.12]\text{ m}$ (shoulder deltoid level)
   - $Z \in [-0.20, 0.06]\text{ m}$ (chest-to-shoulder coronal slice)

## Consequences
- Completely eliminates accidental Eyeshine and flashlight toggling during climbing, mantling, and high melee blocks.
- Reliable, deliberate activation when physically tapping the side of the HMD or clavicle.
