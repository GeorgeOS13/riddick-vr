# ADR 0410: Dialogue Selection Thumbstick Navigation and Unified Confirmation

## Status
Accepted

## Context
1. In ADR 0409, `vrState.cinemaMode` was permanently locked to `false` to completely eradicate the screen-shrinking letterbox system and dual-POV flashing.
2. However, dialogue choice navigation (`VK_UP` / `VK_DOWN` via thumbsticks) and menu confirmation buttons were previously gated behind `if (vrState.cinemaMode)`. With `cinemaMode` set to false, those stick navigation handlers became unreachable.
3. In *The Chronicles of Riddick: Assault on Dark Athena*, conversations with NPCs (Barber, Shmem, Abbott, Johns, etc.) present branching dialogue choices. While player locomotion handles movement in 3D, the Right Controller thumbstick vertical axis ($Y$) is completely unused during gameplay because camera pitch is driven 1:1 by natural physical head tilt.
4. Furthermore, face button mapping had separate branches for cinema mode and gameplay mode. Button A in gameplay mode only sent `'E'`, which could not confirm dialogue selections or menu options in UI contexts that specifically expect `VK_RETURN` (Enter).

## Decision
1. **Dialogue & Menu Navigation via Right Thumbstick Y (`src/hook/camera_hook.cpp`)**:
   - Mapped vertical deflection of the Right Thumbstick ($Y > 0.50$ for `VK_UP`, $Y < -0.50$ for `VK_DOWN`) directly to menu and dialogue option navigation with tactile haptic confirmation ($0.20$ amplitude).
   - Added a center deadband reset threshold ($|Y| < 0.25$) to prevent accidental scrolling or repeated key spam.
   - Because vertical right stick deflection is not used for camera pitch in VR, this provides an intuitive, dedicated dialogue selector that never interferes with horizontal snap/smooth turning.
2. **Unified Action & Dialogue Confirmation on Button A (`src/hook/camera_hook.cpp`)**:
   - Button A now dispatches both `'E'` (primary interact/climb/talk) and `VK_RETURN` (dialogue response confirm, menu select) simultaneously with tactile haptic pulse ($0.35$).
   - In 3D action gameplay, `VK_RETURN` is ignored by the engine's movement state, preserving pure single-action interaction.
   - In dialogue prompts and UI menus, `VK_RETURN` instantly confirms the highlighted dialogue option or menu item.
3. **Dedicated Pause & Back Navigation**:
   - Left Controller Menu Button remains mapped to `VK_ESCAPE`, providing universal pause/resume and menu backing across all game states.
   - Button B remains mapped to Jump (`VK_SPACE`) and Crouch-Jump mantle clearance.
   - Button X remains mapped to Crouch toggle (`'C'`).

## Consequences
- Players can smoothly scroll through NPC dialogue branches using the Right Controller thumbstick and confirm their selection with Button A.
- Eliminated dead input code paths resulting from `cinemaMode = false`.
- Retains 100% full VR FOV edge-to-edge with no screen flashing, dual POVs, or unexpected camera shifts.
- Passes all 60 frames of the autonomous self-play test pipeline with 0 green glitch pixels.
