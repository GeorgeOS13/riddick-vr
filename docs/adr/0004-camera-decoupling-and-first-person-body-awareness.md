# ADR 0004: Camera Decoupling & First-Person Body Awareness

## Status
Accepted

## Context
One of the most defining characteristics of *The Chronicles of Riddick: Escape from Butcher Bay* was its pioneering **first-person body awareness**. Unlike contemporary shooters where the player was a floating camera with detached gun arms, Riddick’s full character model is rendered in first person:
- Looking down reveals Riddick's boots, legs, and torso.
- Riddick leans against walls, climbs ladders, pulls himself into ventilation shafts, and performs context-sensitive stealth takedowns.
- The camera is directly parented to the head/neck bone of Riddick's 3D skeletal rig.

In a flat-screen game, this design provides intense cinematic grit. In VR, however, directly binding the HMD camera to an animated head bone is unplayable:
1. Animated head bobs, weapon recoil sways, and torso rotation twist the horizon, inducing immediate motion sickness.
2. When the player rotates their real head, it clashes with the game's internal orientation state.
3. Ladder and climbing animations force camera rolls and sudden third-person transitions.

*F.E.A.R. VR* encountered this exact phenomenon and solved it by detaching camera pitch/roll from animation rigs while maintaining body presence underneath.

## Decision
1. **Camera Detachment & 6DOF Injection:**
   - Intercept the internal camera matrix generation routine in `DarkAthena.exe`.
   - Strip animation-driven Roll and Pitch while retaining only the global world-space position of the character's neck pivot joint.
   - Inject the user's real-time 6DOF HMD orientation (Yaw, Pitch, Roll) and positional translation delta relative to the neck base.
2. **Torso Yaw Decoupling:**
   - The body model's torso yaw will remain aligned with movement direction / thumbstick look direction, allowing the user to look freely over their shoulder without unnaturally turning the character's hips.
3. **Physical Crouch Detection:**
   - Instead of relying solely on button-triggered crouching, monitor the headset's height relative to the user's calibrated standing height.
   - When the user physically ducks below a defined threshold (e.g. 75% of standing eye height), simulate the game's crouch key state.
4. **Context Transitions (Ladders, Vents, Takedowns):**
   - Provide two player-selectable comfort modes:
     - **Mode A (Immersion):** Maintain first-person 6DOF freedom while Riddick's arms and legs execute the climb/takedown animations below the player's gaze.
     - **Mode B (Comfort / Diorama):** Smoothly transition to a stabilized floating third-person camera during canned climbing and execution animations (similar to *Resident Evil 4 VR*).

## Consequences
### Positive
- Preserves the legendary physical immersion and body presence of Butcher Bay without vestibular conflict.
- Natural head looking around corners and cover without exposing the entire player hitbox.

### Negative / Challenges
- High-intensity melee takedowns may require custom camera clipping offsets to avoid viewing the inside of enemy character meshes.
