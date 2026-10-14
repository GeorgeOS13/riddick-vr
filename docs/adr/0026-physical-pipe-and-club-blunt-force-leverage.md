# ADR 0026: Physical Pipe and Club Two-Handed Blunt Force Leverage and Overhead Slams

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay*, melee weapons include heavy blunt instruments: lead pipes, stun clubs, iron rods, and knuckle dusters.
Unlike cutting blades (Ulaks, shivs) which slice laterally, heavy blunt weapons deal maximum damage through vertical downward momentum and two-handed leverage:
1. **Overhead Slam:** Bringing a lead pipe down on an inmate or guard's head delivers massive concussive trauma and triggers the engine's instant knockdown state.
2. **Kinematic Differentiation:** While single-arm swings perform directional hooks and lunges, a two-handed overhead slam requires raising both hands above chin level and driving them downward simultaneously with full body weight.

## Decision

We implement **Two-Handed Heavy Blunt Kinematics** in Section 5 of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
             [Both Hands Raised High: Y > 0.10m relative to HMD]
             [Both Grips Squeezed: Grip > 0.45f (Clenched on Pipe)]
                                   │
                                   ▼
             [Simultaneous Downward Velocity: Vy < -1.30 m/s]
                                   │
                                   ▼
         [Executes Two-Handed Heavy Blunt Crushing Slam]
         - Dispatches 'S' (Heavy strike modifier)
         - Dispatches Primary Attack (LMB Down / Up)
         - Concussive Blunt Shockwave: 0.95 Amplitude (120 Hz) on BOTH hands
         - Triggers native Starbreeze heavy knockdown
```

### 1. Dual-Arm Trajectory Analysis
- Detects when both controllers are elevated ($Y_{local} > 0.10\text{ m}$), firmly gripped ($> 0.45$), and swung downward with downward velocity $V_{y, left} < -1.30\text{ m/s} \land V_{y, right} < -1.30\text{ m/s}$.

### 2. Concussive Low-Frequency Haptic Waveform
- Generates a heavy $0.95$ amplitude impact pulse at $120\text{ Hz}$ across both hands, providing the visceral feeling of steel pipe impact against armor or bone.

## Consequences

### Positive
- Visceral physical execution of Butcher Bay's iconic lead pipe and club combat.
- Clean distinction between agile knife carving and devastating heavy blunt trauma.
