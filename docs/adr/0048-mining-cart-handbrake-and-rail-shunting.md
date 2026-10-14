# ADR 0048: Sub-Level Mining Cart Handbrake and Rail Shunting Gestures

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* (Work Area & Lower Mining Complex):
1. **Mining Transit:** Riddick navigates extensive subterranean mine shafts riding heavy motorized ore carts on narrow-gauge steel tracks.
2. **Speed Regulation & Rail Switching:** Controlling cart momentum requires pumping the iron brake bar and reaching out to slap track junction switch levers to avoid derailment into magma vents or security barriers.
3. **VR Disconnect:** On desktop, rail riding was largely automated with minimal input. In VR, gripping the vertical brake handle and shoving forward or pulling back gives players direct mechanical control over kinetic velocity.

## Decision

We implement **Mining Cart Kinetic Handbrake & Track Junction Shunting** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Right Hand Gripping Mining Cart Brake Lever]
                   [Z_local in [-0.35, -0.15]m, Grip > 0.50]
                                    │
                                    ▼
           [Forward Shove (Acc) or Backward Pull (Brake): |Vz| > 0.35 m/s]
                                    │
                                    ▼
           [Cart Momentum Modulated ('W' or 'S')]
           - Injects continuous steel wheel rail shudder: 0.40 (60 Hz)
           - Off-hand lateral slap (|Vx| > 0.60 m/s) shunts rail junction ('E')
```

### 1. Longitudinal Brake Bar Modulation
- Detects the dominant hand gripping the vertical lever zone ($Z_{local} \in [-0.35, -0.15]\text{ m}$).
- Forward shove ($V_z < -0.35\text{ m/s}$) opens throttle ('W'); pulling backward ($V_z > +0.35\text{ m/s}$) applies friction brake ('S').

### 2. Rail Shudder & Shunting Haptics
- Continuous low-frequency steel wheel rail shudder ($60\text{ Hz}$ at $0.40$ amplitude) conveys iron cart mass.
- Lateral off-hand reach ($|V_x| > 0.60\text{ m/s}$) triggers rail shunting levers with a crisp $0.65$ switch latch pulse.

## Consequences

### Positive
- Thrilling roller-coaster kinetic presence through the subterranean Butcher Bay mine shafts.
- Direct physical agency over vehicle throttle and braking dynamics.
