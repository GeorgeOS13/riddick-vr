# ADR 0042: Sniper Rifle Diegetic Eye Relief and Bipod Bracing

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*:
1. **Long-Range Marksmanship:** In wide open shipyard bays and space deck arenas, Riddick acquires sniper rifles to eliminate mercenary snipers perched on elevated catwalks.
2. **Diegetic Eye Relief:** Traditional 2D flat scopes black out the entire screen. In VR, bringing the optical scope bell close to the eye ($dist < 12\text{ cm}$) seamlessly magnifies the interior focal plane while preserving situational peripheral vision.
3. **Bipod Surface Bracing:** Resting weapons against catwalk railings or crate surfaces dampens weapon sway for pinpoint accuracy.

## Decision

We implement **Diegetic Eye Relief Scope Zoom & Surface Bracing** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Sniper Scope Raised to Eye Level]
                   [Right Controller HandDist < 12cm from Eye]
                                  │
                                  ▼
           [Diegetic High-Magnification Scope Mode Engaged]
           - Enforces ultra-tight aim smoothing: alpha = 0.12
           - Off-hand resting on railing (< 8cm vertical movement):
             Engages Bipod Lock (alpha = 0.05, 0.20 steady breath pulse)
```

### 1. Close Eye Relief Detection
- Computes controller distance to HMD view origin ($dist < 0.12\text{ m}$).
- Tightens aim smoothing factor to $\alpha = 0.12$ for long-distance stability.

### 2. Physical Railing / Bipod Bracing
- When the off-hand controller is held steady against a simulated railing surface, the system engages bipod lock ($\alpha = 0.05$), eliminating breathing sway and providing sniper perfection.

## Consequences

### Positive
- Authentic, crystal-clear long-range sniping without disruptive full-screen 2D blackout overlays.
- Tactile reliance on physical geometry for steady aim.
