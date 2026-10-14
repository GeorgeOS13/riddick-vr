# ADR 0047: Guard Subdue Chokehold and Silent Neck Snap Gestures

## Status
Accepted

## Context
Across *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena*:
1. **Predatory Stealth Subdual:** Riddick is a master predator who approaches patrolling guards silently from the shadows to neutralize them before an alarm is sounded.
2. **Desktop Mechanics:** In the original PC release, approaching an enemy from behind prompted a context icon; clicking Left Mouse Button initiated an automated neck snap animation.
3. **VR Physicality:** In Virtual Reality, players expect to reach their arms around the victim's neck to initiate a physical grapple chokehold, followed by an aggressive, two-handed cervical twist to silently snap the neck or choke them to sleep.

## Decision

We implement **Rear Stealth Chokehold & Two-Handed Cervical Twist Finishing** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Player Sneaking in Crouch Behind Enemy]
                   [Left Hand Across Chest: Z in [-0.35, -0.15]m, Grip > 0.50]
                                     │
                                     ▼
                   [Rear Chokehold Grapple Initiated]
                   - Injects grapple grab ('E' held)
                   - Emits 100 Hz rhythmic struggle pulse (0.45)
                                     │
                                     ▼
          [Lethal Finish: Rapid Opposing Hand Twist (|Vx| > 0.80 m/s)]
                   - Snaps cervical spine ('LMB' click)
                   - Delivers 0.95 high-frequency bone-cracking crunch (280 Hz)
```

### 1. Rear Grapple Collar Encirclement
- Detects off-hand raised across chest height ($Z_{local} \in [-0.35, -0.15]\text{ m}, Y_{local} \in [-0.20, +0.10]\text{ m}$) with Left Grip clenched while crouched or peeking.
- Emits continuous $100\text{ Hz}$ struggling pulse haptics ($0.45$ amplitude).

### 2. Cervical Twist Finisher
- Detects rapid lateral velocity spike on the dominant hand ($|V_x| > 0.80\text{ m/s}$) or opposing hand twist.
- Injects a Primary Attack click (`LMB`) and triggers a devastating bone-cracking shockwave ($0.95$ amplitude at $280\text{ Hz}$).

## Consequences

### Positive
- Deepens the signature Riddick predator fantasy during shadow infiltration.
- Highly intuitive, visceral motion execution replacing passive button prompts.
