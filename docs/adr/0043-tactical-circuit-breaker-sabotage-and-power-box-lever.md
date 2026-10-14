# ADR 0043: Tactical Circuit Breaker Sabotage and Power Box Lever Gestures

## Status
Accepted

## Context
Across *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena*:
1. **Darkness as a Weapon:** Riddick's Eyeshine gives him complete visual dominance in darkness. In corridors and prison barracks, cutting the main power plunges enemy guards into panic while granting Riddick total stealth concealment.
2. **Wall-Mounted Circuit Breakers:** Power conduits and light breaker panels feature heavy downward pull-levers.
3. **VR Physicality:** In VR, reaching forward to grab the heavy knife-switch lever ($Z_{local} < -0.25\text{ m}$) and wrenching it downward ($V_y < -0.40\text{ m/s}$) plunges the sector into darkness with a heavy electrical trip arc.

## Decision

We implement **Circuit Breaker Downward Pull Kinematics** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Player Reaches to Wall Breaker Box]
                   [Z_local in [-0.45, -0.20]m, Grip Squeezed > 0.50]
                                    │
                                    ▼
           [Downward Lever Pull Motion: V_y < -0.40 m/s]
                                    │
                                    ▼
           [Breaker Tripped / Sector Lights Extinguished]
           - Injects interaction toggle ('E')
           - Delivers 0.80 heavy mechanical contactor clack & arc flash
```

### 1. Lever Grip & Downward Stroke
- Detects hand engagement with the wall breaker plane ($Z_{local} \in [-0.45, -0.20]\text{ m}$).
- Downward yank ($V_y < -0.40\text{ m/s}$) triggers the breaker trip.

### 2. Contactor Snap Haptics
- Delivers a hefty $0.80$ amplitude mechanical clack at $140\text{ Hz}$, tactilely confirming power cut.

## Consequences

### Positive
- Enforces active tactical environmental manipulation for shadow creation.
- Seamlessly sets up stealth ambushes and Eyeshine hunting.
