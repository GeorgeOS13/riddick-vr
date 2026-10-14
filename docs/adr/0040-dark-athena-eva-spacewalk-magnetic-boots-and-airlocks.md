# ADR 0040: Dark Athena EVA Spacewalk Magnetic Boots and Airlocks

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*:
1. **EVA Exterior Hull Spacewalk:** Riddick must traverse the vacuum-exposed outer hull of the Dark Athena dreadnought to circumvent interior blast doors.
2. **Magnetic Boot Clamps:** Without gravity or atmosphere, Riddick stays anchored via magnetic sole actuators that lock onto the hull plating with each stride.
3. **VR Physical Presence:** In VR, stepping across the star-studded exterior requires heavy, rhythmic metallic footfall impacts and atmospheric decompression cues.

## Decision

We implement **EVA Spacewalk Magnetic Boot Actuation** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Player Moving Across EVA Hull Deck]
                   [Thumbstick Locomotion Magnitude > 0.30]
                                  │
                                  ▼
           [Alternating Footfall Stride Cadence: ~500ms Interval]
           - Left Boot Clamped: 0.65 Low-Frequency Magnetic Thud (75 Hz)
           - Right Boot Clamped: 0.65 Low-Frequency Magnetic Thud (75 Hz)
                                  │
                                  ▼
           [Decompression Airlock Cycle ('E' at Hatch)]
           - Dual-controller 0.70 pneumatic venting vibration
```

### 1. Stride Cadence Magnetic Clamping
- Tracks locomotion pace during exterior sections, generating alternating left/right controller pulses ($0.65$ amplitude at $75\text{ Hz}$) that convey magnetic locks latching onto cold hull steel.

### 2. Airlock Cycling Feedback
- Activating airlock equalization valves injects a low-frequency $0.70$ pneumatic venting rumble.

## Consequences

### Positive
- Haunting, breathtaking vacuum spacewalk atmosphere in Dark Athena.
- Unmatched tactile immersion walking along the dreadnought hull overlooking deep space.
