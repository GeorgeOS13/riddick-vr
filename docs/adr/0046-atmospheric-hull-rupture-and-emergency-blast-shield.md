# ADR 0046: Atmospheric Hull Rupture and Emergency Blast Shield Lowering

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*:
1. **Atmospheric Decompression Hazards:** Breaches in the outer hull of the Dark Athena dreadnought cause violent explosive decompression, pulling objects and air towards open space.
2. **Emergency Blast Shield:** Surviving a breach requires reaching an emergency lockdown station and yanking down the dual-lever overhead blast shield mechanism.
3. **VR Physicality:** In VR, reaching both hands overhead ($Y_{local} > +0.25\text{ m}$) and pulling down hard ($V_y < -0.50\text{ m/s}$) drops the heavy steel barrier into place, sealing the compartment.

## Decision

We implement **Dual-Hand Blast Shield Overhead Pull Kinematics** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Both Hands Overhead at Blast Station]
                   [Y_local > +0.25m, Grips Held > 0.50]
                                    │
                                    ▼
           [Simultaneous Downward Pull: Vy_left & Vy_right < -0.50 m/s]
                                    │
                                    ▼
           [Emergency Blast Shield Slammed Shut]
           - Injects emergency sealing pulse ('E')
           - Delivers dual-hand 0.95 heavy bulkhead ground slam haptic
```

### 1. Dual-Hand Overhead Latch
- Detects both controllers raised above head level ($Y_{local} > +0.25\text{ m}$) with both grips clenched.

### 2. Downward Slam & Hydraulic Lock
- Simultaneous downward stroke ($V_y < -0.50\text{ m/s}$) slams the blast shield shut, dispatching a crushing $0.95$ amplitude impact shockwave.

## Consequences

### Positive
- Heroic, high-stakes cinematic moments during shipboard depressurization catastrophes.
- Intuitive physical response to environmental decompression alarms.
