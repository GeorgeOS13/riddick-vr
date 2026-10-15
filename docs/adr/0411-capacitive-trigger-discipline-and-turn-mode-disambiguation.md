# ADR 0411: Capacitive Trigger Discipline & Turn Mode Recenter Disambiguation

## Context
1. In ADR 0406, always-visible 3D procedural tactical gloves (`VRHandRenderer`) were introduced to provide continuous first-person physical presence tracking motion controllers 1:1 in 6DOF.
2. However, the initial finger curling implementation tied the index finger's curl directly to `effectiveGrip > 0.45f`. When the player gripped a weapon handle or object, their index finger was forced into a clenched fist regardless of whether they were touching or pulling the trigger. This broke realistic trigger discipline and caused the index finger to clip through weapon trigger guards.
3. In `CameraHook::Update`, Right Thumbstick Click was mapped to recentering the HMD view, resetting the standing eye height baseline, and pulsing haptics. Simultaneously, toggling between Snap Turn (45°) and Smooth Turn (cubic curve) was assigned to `Left Grip + Right Stick Click`. Because the recenter check did not exclude `leftGripHeld`, toggling the turn mode unintentionally triggered a full view recenter and recalibrated the player's floor height.

## Decision
1. **Capacitive Trigger Discipline in `VRHandRenderer::DrawTacticalHand`**:
   - Passed controller `buttons` bitmask into `DrawTacticalHand`.
   - Detected capacitive touch states:
     - `triggerTouch = (buttons & RIDDICK_VR_TOUCH_TRIGGER) != 0 || trigger > 0.05f`.
     - `thumbTouch = (buttons & (RIDDICK_VR_TOUCH_THUMBSTICK | RIDDICK_VR_TOUCH_THUMBREST | RIDDICK_VR_BTN_A | RIDDICK_VR_BTN_B | RIDDICK_VR_BTN_X | RIDDICK_VR_BTN_Y)) != 0`.
   - Articulation states:
     - **Boxer Guard (`isBoxerGuard == true`)**: Index finger curls to `0.92f`, thumb to `0.88f`, creating a solid clenched fist.
     - **Weapon Grip / Non-Touch (`!triggerTouch`)**: Index finger remains fully extended (`indexCurl = 0.0f`) along the barrel/receiver, maintaining proper tactical trigger discipline.
     - **Resting on Trigger (`triggerTouch && trigger <= 0.05f`)**: Index finger adopts a relaxed resting curvature (`indexCurl = 0.15f`).
     - **Active Trigger Pull (`trigger > 0.05f`)**: Index finger curls proportionally (`0.15f + 0.80f * trigger`).
     - **Thumb Resting vs Lifting**: Thumb relaxes outward when lifted off buttons/sticks (`0.08f`), and rests naturally on the control surface when touched (`0.35f + 0.50f * grip`).

2. **Disambiguation of Turn Mode Toggle and View Recentering in `CameraHook::Update`**:
   - Gated the Right Thumbstick Click recenter trigger behind `!leftGripHeld`:
     ```cpp
     if (stickRight && !prevStickRight && !leftGripHeld) {
         vrState.shouldRecenter = true;
         ...
     }
     ```
   - This ensures that pressing Right Stick Click while holding Left Grip cleanly toggles Snap Turn vs Smooth Turn mode without recentering the headset orientation or resetting the standing height baseline.

## Consequences
- 3D tactical gloves feature authentic VR trigger discipline: index finger points forward or rests along the barrel until the trigger is touched or squeezed.
- Toggling snap vs smooth turning is isolated and will never disorient the player by suddenly resetting their tracking origin.
- Zero green glitch pixels, verified with `tools/self_test_pipeline.py`.
