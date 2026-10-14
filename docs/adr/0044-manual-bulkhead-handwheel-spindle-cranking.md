# ADR 0044: Manual Bulkhead Handwheel Spindle Cranking Gestures

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* (Sewer Systems & Mining Shafts) and *Assault on Dark Athena* (Reactor Core):
1. **Manual Blast Doors:** Unpowered heavy pressure bulkheads must be opened manually via circular handwheel spindles.
2. **Desktop Mechanics:** Pressing 'E' played an unskippable canned turning animation.
3. **VR Physicality:** In VR, grabbing the handwheel rim and executing continuous circular wrist/arm revolutions unthreads hydraulic dogging locks.

## Decision

We implement **Circular Spindle Cranking Recognition** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Hand Gripping Bulkhead Wheel Rim]
                   [Z_local in [-0.45, -0.22]m, Grip > 0.50]
                                    │
                                    ▼
           [Circular Hand Revolutions: |Vx*Vy| > 0.15, R ~ 15cm]
                                    │
                                    ▼
           [Spindle Cranking Step Executed]
           - Injects door unsealing pulse ('E')
           - Delivers periodic mechanical dogging ratchet clicks: 0.45 (160 Hz)
```

### 1. Spindle Contact & Tangential Velocity Tracking
- Measures circular motion in the frontal X-Y plane while holding the wheel plane ($Z_{local} \in [-0.45, -0.22]\text{ m}$).

### 2. Ratchet Tooth Indexing
- Emits sharp $0.45$ amplitude ratchet clicks at each quarter-turn interval, conveying hydraulic dogs sliding back.

## Consequences

### Positive
- Physical effort and weight when breaching sealed quarantine bulkheads.
- High adrenaline when manually cranking open emergency doors while being pursued by guards.
