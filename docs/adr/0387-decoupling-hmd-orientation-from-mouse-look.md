# ADR 0387: Decoupling HMD Head Orientation from Windows Mouse Look

## Context
In stereoscopic VR rendering, 6DOF head tracking is applied directly to the in-engine camera matrix (`StabilizeViewMatrix`) by multiplying the stabilized camera base by `eyeMat` (`vrState.leftEyeView` / `vrState.rightEyeView`).
However, `CameraHook::Update` was simultaneously calculating incremental head yaw and pitch (`deltaYaw`, `deltaPitch`) and injecting them via `mouse_event(MOUSEEVENTF_MOVE)`. In addition, an audio listener yaw synchronization loop was feeding yaw errors directly back into `mouseDeltaX`.
This resulted in double rotation on every head turn, violent camera jerks, and camera spinning in every direction during gameplay.

## Decision
1. Completely remove `deltaYaw` and `deltaPitch` from the `mouse_event` look pipeline. Head orientation is strictly resolved through 6DOF view matrix stabilization.
2. Remove the unstable audio listener yaw correction feedback loop from `mouseDeltaX`.
3. Restrict `mouse_event` look injection exclusively to intentional body turning via the right thumbstick (discrete snap turn or cubic smooth turn) and subtle deadband controller aim pull when actively aiming down sights or holding a two-handed weapon.
4. Gate thumbstick turning by `!vrState.cinemaMode` so that stick navigation does not inadvertently rotate the character body in menus.

## Consequences
- Eliminates camera over-rotation, jerking, and disorientation.
- Head tracking is 1:1, silky smooth, and stable.
- Weapon aim and body turning remain intuitive via motion controller aiming and thumbstick turning.
