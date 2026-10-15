# ADR 0406: Always-Visible 3D VR Hands / Tactical Gloves and Diegetic Agency

## Status
Accepted

## Context
1. In the pancake release of *The Chronicles of Riddick: Assault on Dark Athena*, character hands/arms are only rendered when specific weapons or fist items are equipped (`CRPG_Object_Item`, `CWO_Clientdata_WeaponAnim_AnimGraph2`). When unarmed, idle, or carrying environmental objects, the engine sets `noviewmodel` to suppress first-person meshes entirely.
2. During interactive environmental sequences (NanoMED medical healing stations, ladder climbing, pipe monkey-bars, vent grates, and execution takedowns), interactive entities (`Action_CutsceneAlphaDroneKill`, `CWObject_InputEntity`) invoke `usethirdperson` and force the camera into a detached third-person perspective with canned, non-interactive animations.
3. In Virtual Reality, player agency demands continuous physical embodiment (mimicking the design paradigm of *F.E.A.R. VR*). The player's hands must be **always visible in 3D**, tracking motion controllers 1:1 in 6DOF space at all times, without being hidden by `noviewmodel` flags or interrupted by third-person cinematic locks.
4. Physical presence requires interactive dynamic hand postures:
   - **Open Tactical Palm:** Relaxed finger extension for flat-palm contact on NanoMED healing stations (`rightPalmOpen`, `leftPalmOpen`) and biometric terminal poke interactions.
   - **Articulated Grip:** Finger curling driven continuously by analog grip trigger input (`grip > 0.40`) for grasping ladder rungs, pipes, shivs, and weapons.
   - **Trigger Articulation:** Index finger curling driven by analog trigger input (`trigger > 0.40`) for firearm trigger control.
   - **Clenched Power Fist / Boxer Guard:** Complete 4-finger and thumb closure forming a reinforced combat fist when clenching fists or raising arms into Boxer Guard, leading with the reinforced knuckle guard plate.

## Decision
1. **Dedicated Procedural 3D VR Hand Renderer (`VRHandRenderer`):**
   - Implemented a standalone, zero-dependency 3D tactical glove renderer in `src/hook/vr_hand_renderer.h` and `src/hook/vr_hand_renderer.cpp`.
   - Each hand model features:
     - **Forearm Restraint Gauntlet:** Tapered vambrace sleeve with metallic reinforcement bands and dorsal armor plate, aligning seamlessly beneath the Diegetic Wrist Health HUD (`wristQuadLayer` at local offset `{-0.01m, +0.045m, +0.075m}`).
     - **Contoured Palm:** Multi-faceted carpal/metacarpal body with ribbed palmar friction grip pad.
     - **Reinforced Boxer Knuckle Guard:** Hardened composite 4-lobed striking ridge across metacarpal heads.
     - **Articulated Digits:** Anatomically fanned digits (Index -4.5°, Middle 0°, Ring +3°, Pinky +6.5°) with 3-segment hierarchical joint articulation (proximal, intermediate, distal) driven dynamically by analog grip and trigger inputs.
     - **Opposable Articulated Thumb:** Dual-segment thumb that folds naturally over the curled fingers when gripping or punching.
2. **Eye Framebuffer Integration (`stereo_renderer.cpp`):**
   - Directly integrated into `StereoRenderer::OnSwapBuffers(HDC hdc)`:
     - Left Eye: Rendered into `m_leftFbo` immediately following the scene backbuffer blit.
     - Right Eye: Rendered into `m_rightFbo` immediately following the scene backbuffer blit.
   - Hand rendering utilizes a dedicated cleared depth buffer pass, preventing tactical hands from ever clipping into distant geometry or world walls.
   - Projection focal length and winding order (`glFrontFace(GL_CW)`) are calibrated to match the top-down D3D11/OpenGL interop coordinate system.
3. **OpenXR Tracking & Self-Play Verification (`openxr_manager.cpp`):**
   - Fully utilizes `state.leftController.gripPose` and `state.rightController.gripPose` in 6DOF space.
   - Simulated host mock mode provides dynamic finger curling and pose validation for headless CI/CD pipelines.

## Consequences
- Player agency is permanently maintained with continuous 3D hand presence in VR.
- Full physical presence is established for NanoMED palm touch, ladder grasping, Boxer Guard blocking, and punching strikes.
- Zero link conflicts or external runtime dependencies; builds cleanly and runs in real time at native VR refresh rates.
