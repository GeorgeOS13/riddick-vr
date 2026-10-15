# ADR 0395: Aim-Pull and Feedback Loop Elimination from Camera Mouse Look

## Context
Players reported severe camera disorientation in gameplay: the view moved uncontrollably and spun in every direction. Investigation of `CameraHook::Update` identified two major defects:
1. Motion Controller Aim-Pull Mouse Injection: When the player engaged aiming or two-handed grip, `pullYaw` and `pullPitch` calculated angular differences between controller orientation and head orientation and called `mouse_event(MOUSEEVENTF_MOVE, moveX, moveY)` every single frame at 90 FPS. Holding the weapon slightly off-center continuously injected mouse deltas, causing the camera to spin in circles and force the view toward the floor or ceiling.
2. Unstable Audio Listener Feedback Loop: An accumulated error between HMD yaw and engine listener yaw was scaled by 53 pixels per frame and injected into `mouseDeltaX`, creating an oscillatory feedback loop that shook and spun the view.

## Decision
1. Completely eliminate motion controller aim-pull (`pullYaw`, `pullPitch`) from camera mouse injection. In VR, motion controllers must point independently in 6DOF space without rotating or pulling the player's head view.
2. Remove the audio listener yaw feedback correction loop from mouse movement calculations.
3. Provide pure, filtered 1:1 HMD head tracking look via clean `deltaYaw` and `deltaPitch` with a center deadband ($0.0010\text{ rad} \approx 0.06^\circ$) to eliminate high-frequency sensor tremor.
4. Keep discrete 45° snap turning and cubic smooth turning strictly bound to right thumbstick horizontal deflection.

## Consequences
- Eliminates camera runaway rotation, spinning, and random pitch tilting to the floor/ceiling during combat.
- Head tracking look feels rock-solid, smooth, and 1:1 with player head movements.
- Hands can aim freely in 6DOF without affecting camera stability.
