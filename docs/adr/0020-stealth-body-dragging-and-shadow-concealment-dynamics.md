# ADR 0020: Stealth Body Dragging and Kinematic Drag Friction Dynamics

## Status
Accepted

## Context
Stealth gameplay in *The Chronicles of Riddick* requires hiding evidence of infiltration:
1. **Concealing Neutralized Guards:** Dragging unconscious inmates or dead mercenaries into dark alcoves and ventilation shafts to avoid alerting patrol routes and security drones.
2. **Heavy Debris Positioning:** Pulling metal crates or industrial machinery into position to climb into overhead catwalks.

In the original flat game, players held `E` while walking backwards or sideways to haul bodies. In VR, holding a body while walking backwards should feel physically encumbering, providing tangible tactile weight and friction feedback.

## Decision

We implement **Kinematic Drag Friction Resistance** in Section 9 of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Off-Hand Grip Held (s_leftGripDown)]
                                     │
                                     ▼
               [Stealth State: Physical Crouch / Wall Peek]
                                     │
                                     ▼
               [Locomotion: Thumbstick Magnitude > 0.25]
                                     │
                                     ▼
         [Emit Periodic Drag Friction Pulses (every 450ms)]
         - Amplitude: 0.38
         - Frequency: 85 Hz (Low-frequency heavy rumble)
         - Simulates heavy physical friction across floor
```

### 1. Motion & Load Tracking
- Evaluates when the player is physically crouched (`s_isPhysicallyCrouched || s_isStealthPeeking`), maintaining Left Grip (`s_leftGripDown`), and actively maneuvering with the thumbstick ($\|V_{loco}\|_1 > 0.25$).

### 2. Low-Frequency Tactile Resistance
- Every $450\text{ ms}$, the system injects an $85\text{ Hz}$ tactile rumble ($0.38$ amplitude) into the off-hand controller.
- Conveys the rhythmic, heavy drag resistance of limbs and torso scraping across grated steel decks or concrete surfaces.

## Consequences

### Positive
- Heavy tactile immersion when dragging corpses or moving crates into shadows.
- Enhances tension during stealth sequences as players pull bodies away from incoming patrol searchlights.
