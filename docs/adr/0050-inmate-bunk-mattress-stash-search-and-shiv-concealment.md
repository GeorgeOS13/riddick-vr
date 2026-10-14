# ADR 0050: Inmate Bunk Mattress Stash Search and Shiv Concealment

## Status
Accepted

## Context
Across Butcher Bay's cell complexes (Single-Max, Double-Max, and Tower 17):
1. **Prison Stash Locations:** Inmates hollow out metal bed tubes and hide illegal shivs, UD bankrolls, and cigarettes beneath thin foam mattresses.
2. **Desktop Mechanics:** Pressing 'E' facing a bunk triggered a generic search notification.
3. **VR Physicality:** In VR, crouching beside a bunk, lifting the mattress corner with the off-hand, and sweeping the dominant hand underneath into the dark hollow creates authentic prison contraband looting.

## Decision

We implement **Mattress Lift & Under-Bunk Hand Sweep Kinematics** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Player Crouched Beside Prison Bunk]
           [Left Hand Lifting Mattress: Y_local in [-0.55, -0.25]m, Grip > 0.45]
                                    │
                                    ▼
           [Right Hand Sweeping Under Bunk: Z < -0.30m, |Vx| > 0.35 m/s]
                                    │
                                    ▼
           [Contraband Stash Discovered / Retrieved]
           - Injects stash collection pulse ('E')
           - Delivers 0.60 metallic discovery chime haptic (200 Hz)
```

### 1. Dual-Hand Bunk Interaction Coordination
- Detects the off-hand holding the mattress edge ($Y_{local} \in [-0.55, -0.25]\text{ m}$) while crouched.
- Dominant hand sweeping underneath ($Z_{local} < -0.30\text{ m}, |V_x| > 0.35\text{ m/s}$) indexes the hidden stash.

### 2. Tactile Item Discovery
- Fires a clean $0.60$ amplitude chime at $200\text{ Hz}$, confirming acquisition of hidden shivs or currency.

## Consequences

### Positive
- Deepens prison roleplay and exploratory scavenge loops.
- Natural, physical coordination between hands during search actions.
