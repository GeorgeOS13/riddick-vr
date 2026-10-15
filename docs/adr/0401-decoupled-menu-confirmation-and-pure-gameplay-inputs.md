# ADR 0401: Decoupled Menu Confirmation & Pure Gameplay Inputs

## Context
1. In `src/hook/camera_hook.cpp`, face button inputs previously injected compound keystrokes indiscriminately across both menus and 3D action gameplay:
   - **Button A:** Injected `E`, `VK_RETURN`, `VK_SPACE`, and `MOUSEEVENTF_LEFTDOWN` simultaneously. During gameplay, pressing Button A to speak with an NPC or interact with an object caused the player character to jump (`Space`) and punch/fire a weapon (`LMB`).
   - **Button B:** Injected `VK_SPACE`, `VK_ESCAPE`, `VK_BACK`, and `VK_TAB` simultaneously. Pressing Button B to jump in gameplay accidentally opened the pause menu (`Escape`) or toggled the inventory PDA (`Tab`).
   - **Button X:** Injected `E`, `VK_RETURN`, and `C` simultaneously, triggering unintended object activations when toggling crouch.
   - **Left Menu Button:** Injected `Escape`, `Backspace`, `Tab`, and `Space`, causing menu selections to activate upon pause.
2. In cinema/menu mode (`vrState.cinemaMode`), thumbstick movement was leaking into `WASD` locomotion, and 4-way arrow navigation was restricted only to the right stick's Y-axis.

## Decision
1. **Mode-Isolated Face Buttons:**
   - **Cinema / Menu Mode (`vrState.cinemaMode`):**
     - Button A: Select / Confirm (`VK_RETURN` + `MOUSEEVENTF_LEFTDOWN/UP`).
     - Button B: Back / Cancel (`VK_ESCAPE` + `VK_BACK`).
     - Button X: Secondary Confirm (`VK_RETURN`).
   - **3D Gameplay Action Mode (`!vrState.cinemaMode`):**
     - Button A: Primary Interact / Use / Talk / Climb / Open ('E'). Zero `Space` or `LMB` cross-talk.
     - Button B: Jump (`VK_SPACE`) and athletic crouch-jump mantle clearance (`C` + `Space`). Zero `Escape` or `Tab` cross-talk.
     - Button X: Crouch Toggle ('C'). Zero `E` or `Return` cross-talk.
2. **Clean Pause Menu Button:**
   - Left Controller Menu Button strictly taps `VK_ESCAPE` for unambiguous menu toggle without spurious `Space`, `Tab`, or `Backspace` inputs.
3. **Dual-Thumbstick 4-Way Menu Navigation:**
   - In cinema mode, gate `WASD` locomotion off.
   - Map either thumbstick:
     - Vertical deflection ($|Y| > 0.45$): `VK_UP` / `VK_DOWN` row navigation.
     - Horizontal deflection ($|X| > 0.45$): `VK_LEFT` / `VK_RIGHT` column / slider navigation.
     - Tactile $0.20$ tick feedback on each navigation step.

## Consequences
- Pressing Button A to interact or talk never punches NPCs or jumps.
- Pressing Button B to jump never opens the pause menu or inventory.
- Menu navigation on both thumbsticks is fluid, responsive, and intuitive.
- Zero spurious input leakage between menu and gameplay contexts.
