# ADR 0041: Pit Fighting Arena Ring Ropes and Concussive Shoves

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* (Tower 17 Pit Fighting League):
1. **The Fighting Pit:** Riddick fights his way up the inmate champion hierarchy (facing Bam, Harman, Rust, and Centurion) in a fenced gladiatorial cage.
2. **Ring Boundary Dynamics:** Inmates bounce off the chainlink walls or cage ropes during intense melee brawls.
3. **VR Physicality:** In VR, throwing an aggressive two-handed push against an opponent or recoiling off the cage boundary requires spring-loaded elastic bounce feedback.

## Decision

We implement **Dual-Palm Concussive Shoves & Ring Boundary Spring Haptics** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Dual Fists / Palms Thrust Forward]
               [Vz_left < -1.10 m/s AND Vz_right < -1.10 m/s]
                                  │
                                  ▼
           [Concussive Two-Handed Shove / Ring Wall Rebound]
           - Injects native heavy melee shove pulse ('W' + 'LMB')
           - Triggers dual-hand 0.85 spring recoil haptic pulse (140 Hz)
```

### 1. Dual-Hand Thrust Kinematics
- Measures simultaneous forward velocity on both controllers ($V_{z} < -1.10\text{ m/s}$ in local space).

### 2. Spring Recoil Haptics
- Dispatches a heavy concussive shove impulse and generates dual $0.85$ amplitude spring rebound haptics, simulating staggering opponents against the arena cage.

## Consequences

### Positive
- Electrifying bare-knuckle brawl physics in the Tower 17 fighting ring.
- Provides a decisive offensive spacing tool to shove back aggressive inmate champions.
