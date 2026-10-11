# 64-bit OpenXR Host Component (`RiddickVRHost.exe`)

## Responsibility
The Host component is an independent 64-bit (x64) application responsible for all direct communication with the VR runtime.

## Key Duties
- Initialize the OpenXR instance and session with the active system runtime (SteamVR, Meta Link, Virtual Desktop).
- Create OpenXR swapchains matching the recommended per-eye render resolutions.
- Poll OpenXR Action Sets for motion controller tracking (poses, buttons, thumbsticks, triggers).
- Maintain frame pacing (`xrWaitFrame`, `xrBeginFrame`, `xrEndFrame`).
- Receive shared GPU textures from the 32-bit game hook and submit them to the OpenXR swapchain.
