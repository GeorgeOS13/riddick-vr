# ADR 0386: Motion Controller Capacitive Touch State & Hand Gesture Disambiguation

## Status
Accepted

## Context
Modern VR controllers (Quest 2/3 Touch, Valve Index Knuckles) feature capacitive touch sensors across the thumbsticks, trigger surfaces, and grip handles. In VR input processing, players frequently rest their index fingers and thumbs on these surfaces without intending to perform an active squeeze or button press.

In *The Chronicles of Riddick* VR mod, environmental interactions such as ladder climbing (`SendKeyTap('E')`) previously tested only the analog grip value ($> 0.50$). If a player raised their hands with open fingers while casually resting against the controller grips, ladder grab logic could trigger unintentionally.

## Decision
Integrate capacitive touch states into the IPC protocol and gesture validation pipelines:

1. **Protocol Extension (`include/riddick_vr_protocol.h`)**:
   - Define capacitive touch bitflags within the controller `buttons` bitmask:
     ```cpp
     constexpr uint32_t RIDDICK_VR_TOUCH_TRIGGER    = 1 << 7;
     constexpr uint32_t RIDDICK_VR_TOUCH_THUMBSTICK = 1 << 8;
     constexpr uint32_t RIDDICK_VR_TOUCH_THUMBREST  = 1 << 9;
     ```

2. **Host Input Polling (`src/host/openxr_manager.cpp`)**:
   - Poll capacitive touch states from OpenXR action paths or synthesize them when trigger/thumbstick contact is detected.
   - Forward touch bitflags across the high-speed shared memory bridge.

3. **Gesture Disambiguation (`src/hook/camera_hook.cpp`)**:
   - For ladder climbing rung grabs, require active capacitive finger curl (`leftTouchEngaged` / `rightTouchEngaged`) in addition to grip force ($> 0.50$).
   - Relaxed hands or pointing finger poses are prevented from initiating inadvertent ladder climbs.

## Consequences
- Accurate disambiguation between resting hand postures and deliberate physical grasping.
- Prevents accidental ladder mounts and interaction misfires.
- Paves the way for natural skeletal finger tracking in first-person rendering.
