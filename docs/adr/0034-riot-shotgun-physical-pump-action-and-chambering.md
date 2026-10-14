# ADR 0034: Riot Shotgun Physical Pump Action and Chambering Gestures

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena*:
1. **The Riot Shotgun:** A devastating close-quarters firearm firing 12-gauge buckshot. It features an under-barrel tubular magazine and a sliding forend pump.
2. **VR Expectation:** In standard desktop play, the shotgun pump animation plays automatically or via a keypress. In VR, players expect two-handed manual pump action: racking the slide back to eject empty hulls and pushing forward to chamber a live shell.

## Decision

We implement **Two-Handed Shotgun Foregrip Pump Action** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Two-Handed Firearm Stance Active]
                   [Off-Hand at Forend: Distance < 28cm]
                                    │
                                    ▼
             [Phase 1: Slide Pulled Rearward (Delta Z > +0.07m)]
             - Ejects spent 12-gauge brass hull
             - Tactile Mechanical Slide Ejection Click (0.55 Amplitude)
                                    │
                                    ▼
             [Phase 2: Slide Pushed Forward (Delta Z < +0.02m)]
             - Chambers fresh buckshot shell into breach
             - Tactile Chamber Lock Snap (0.75 Amplitude)
             - Signals weapon ready / chamber primed
```

### 1. Relative Forend Translation Tracking
- Tracks the off-hand's relative position along the dominant hand's weapon axis during two-handed stabilization.
- Detects the rearward stroke ($\Delta Z_{rel} > +0.07\text{ m}$) as hull ejection.

### 2. Chamber Lock & Dual-Phase Haptics
- Ejection stroke fires a crisp mechanical click ($0.55$ amplitude).
- Forward racking stroke ($\Delta Z_{rel} < +0.02\text{ m}$) locks the bolt, delivering a solid $0.75$ amplitude chamber lock.

## Consequences

### Positive
- Highly tactile, satisfying shotgun gunplay matching modern VR shooter standards.
- Natural physical rhythm between blasts, ducking into cover to pump.
