# ADR 0038: Contraband Smuggling and Ankle Pouch Inventory

## Status
Accepted

## Context
Across *The Chronicles of Riddick: Escape from Butcher Bay*:
1. **Prison Economy:** Inmates trade contraband (UD currency, loose cigarettes, shivs, narcotics) beneath the noses of Butcher Bay guards and security cameras.
2. **Concealed Stashes:** Riddick hides key contraband items in his boot tops and ankle linings to avoid detection during cell sweeps.
3. **VR Physicality:** In VR, reaching all the way down to one's ankle/boot ($Y_{local} < -0.65\text{ m}$) while crouched provides a stealthy physical stash compartment.

## Decision

We implement **Deep Ankle Boot Contraband Holster** in Section 6c of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Player Physically Crouched / Prone]
              [Hand Reaching Down to Boot: Y_local < -0.65m]
                                │
                                ▼
                   [Left or Right Grip Squeezed > 0.50]
                                │
                                ▼
             [Contraband Ankle Pouch Accessed]
             - Injects subtle leather/fabric rustle pulse: 0.35 Amplitude (140 Hz)
             - Toggles contraband/special item inventory slot
```

### 1. Low Ankle Kinematic Zone
- Evaluates controller height relative to HMD when reaching down below the knees ($Y_{local} < -0.65\text{ m}$).

### 2. Fabric Rustle Feedback
- Emits a gentle $0.35$ amplitude pulse at $140\text{ Hz}$, tactilely confirming that contraband has been secured or retrieved from the boot lining.

## Consequences

### Positive
- Deepens prison infiltration and smuggling dynamics.
- Perfectly complements the physical crouching and deep prone mechanics (ADR 0005 & ADR 0030).
