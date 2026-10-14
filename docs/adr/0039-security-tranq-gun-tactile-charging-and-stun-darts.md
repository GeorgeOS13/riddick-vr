# ADR 0039: Security Tranq Gun Tactile Charging and Stun Darts

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay*:
1. **Non-Lethal Stealth:** In Facility A and the Double-Max security sections, Riddick uses the prototype security tranquilizer gun to silently neutralize guards without alerting nearby security cameras.
2. **Capacitor Overcharge:** The tranq gun features a capacitor charge mechanism: holding the trigger builds an electric voltage potential before discharge for an instant knockdown stun.
3. **VR Physicality:** In VR, feeling the electrostatic capacitor hum escalate through the controllers as the voltage builds delivers immense tactical satisfaction.

## Decision

We implement **Electrostatic Capacitor Charge Ramping** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Tranq Gun Trigger Held > 0.40]
                                  │
                                  ▼
           [Capacitor Charging Phase: Elapsed Time 0 to 800ms]
           - Frequency modulates from 80 Hz up to 260 Hz
           - Amplitude ramps proportionally: 0.20 -> 0.70
                                  │
                                  ▼
                     [Trigger Full Release / Peak Fire]
           - Discharges high-voltage stun dart
           - Delivers 0.85 electrical discharge haptic pulse
```

### 1. Progressive Charge Time Tracking
- Measures continuous trigger depression duration during aiming.
- Escalates tactile vibration frequency from $80\text{ Hz}$ to $260\text{ Hz}$ across an $800\text{ ms}$ ramp.

### 2. Discharge Snap
- Releasing the trigger at peak charge releases the dart with an energetic $0.85$ amplitude tactile discharge snap.

## Consequences

### Positive
- Elevated tactical stealth gameplay in Butcher Bay's maximum security sectors.
- Precise physical control over capacitor charge states.
