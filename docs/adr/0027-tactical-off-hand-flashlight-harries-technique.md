# ADR 0027: Tactical Off-Hand Flashlight Parenting and Harries Shooting Stance

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*, dark ventilation shafts and decommissioned cargo decks feature pitch-black corridors where guards and hostile drones lurk.
1. **Low-Light Gunplay:** Operating a sidearm while illuminating targets with a flashlight requires a stable tactical shooting posture.
2. **Harries Technique:** The real-world military/law-enforcement Harries technique involves crossing the wrists (placing the backs of the hands together with the flashlight beneath the firearm frame), locking both wrists to steady the weapon while casting light directly along the bore axis.

## Decision

We implement **Harries Stance Detection & Enhanced Jitter Dampening** in Section 6b of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Tactical Flashlight Active ('F')]
                                   │
                                   ▼
             [Wrist Proximity < 18cm & Off-Hand Grip > 0.35]
                                   │
                                   ▼
              [Tactical Harries Shooting Stance Engaged]
              - Tactile Lock Pulse: 0.35 on Both Controllers
              - Enhanced Aim Smoothing: Alpha = 0.22 (vs 0.35)
              - Locks Flashlight Beam Parallel to Firearm Bore Axis
```

### 1. Spatial Wrist Crossing Kinematics
- Evaluates when `s_flashlightActive` is true, distance between controllers is $< 18\text{ cm}$, and off-hand grip is lightly held ($> 0.35$).
- Confirms engagement via subtle dual-controller tactile pulse ($0.35$).

### 2. High-Precision Aim Smoothing
- Tightens the SLERP smoothing factor from $0.35$ down to $0.22$, filtering micro-tremors and providing laser-steady aim during tense low-light stealth engagements.

## Consequences

### Positive
- Intuitive, professional tactical gunplay in low-light environments.
- Reward players for using real-world tactical shooting techniques.
