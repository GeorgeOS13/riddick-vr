# ADR 0397: Foregrip Trigger and Sprint Stick Key Injection Removal

## Context
During combat and locomotion, players experienced unwanted key presses and flickering:
1. `CameraHook::Update` checked `bool foregripLaserPad = (vrState.leftController.trigger > 0.50f) || ((vrState.leftController.buttons & RIDDICK_VR_BTN_STICK_LEFT) != 0);`. Because pulling the off-hand trigger is the primary action when aiming a two-handed weapon, aiming continuously sent `SendKeyTap('F')` every 350ms, causing the flashlight to flicker wildly on and off.
2. An automated DNA biometric circuit check (`insulatedTapeWrap`) continuously sent `SendKeyTap('E')` whenever hands were held close together with grip engaged, accidentally triggering object interactions, holster commands, and dialogue interruptions.
3. Left thumbstick click (`RIDDICK_VR_BTN_STICK_LEFT`) was wired to sprint toggle at line 802, but simultaneously called `SendKeyTap('C')` (crouch) at line 10342. Clicking the stick to sprint immediately forced Riddick into a crouch, which canceled sprinting.

## Decision
1. Remove `foregripLaserPad` trigger checks and associated automated key taps from two-handed gunplay. Tactical flashlight toggle is handled cleanly via the deliberate shoulder tap gesture ($Y \in [-0.10, 0.12]\text{ m}$, $X \in [-0.28, -0.12]\text{ m}$, grip held).
2. Remove automated `SendKeyTap('E')` checks during two-handed weapon aiming.
3. Remove `SendKeyTap('C')` from left stick click so clicking the stick exclusively toggles forward sprint (`VK_SHIFT`). Crouch remains mapped to Button X and physical crouch stance.

## Consequences
- Aiming with two-handed firearms never causes the flashlight to flicker.
- Interacting ('E') is strictly driven by intentional button presses and deliberate gestures.
- Left thumbstick click sprint activates cleanly without forcing the character into crouch.
