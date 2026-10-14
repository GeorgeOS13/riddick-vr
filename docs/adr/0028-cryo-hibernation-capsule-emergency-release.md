# ADR 0028: Cryo-Hibernation Capsule Emergency Seal Ejection and Pod Escape

## Status
Accepted

## Context
The narrative of *Assault on Dark Athena* begins with Riddick awakened from cryo-sleep inside a hibernation capsule aboard his vessel as the mercenary ship *Dark Athena* snares them:
1. **Escape Sequence:** Riddick must force open the pod glass canopy to exit before boarders breach the ship.
2. **Visceral VR Presence:** Rather than passively waiting through a canned cutscene or tapping a keyboard button, pushing both hands against the interior pod canopy to blow the pneumatic seals establishes intense physical presence from the very first seconds of the game.

## Decision

We implement **Dual-Palm Pod Canopy Ejection** in Section 6d of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Player Encapsulated in Pod / Duct]
                                   │
                                   ▼
             [Both Open Palms Extended Forward: Z < -0.22m]
             [Simultaneous Forward Shove: Vz_left < -0.28 & Vz_right < -0.28]
                                   │
                                   ▼
          [Pneumatic Seal Decompression & Canopy Ejection]
          - Dispatches 'E' + Space
          - Dual Pneumatic Decompression Surge: 0.80 Amplitude
          - Pod canopy flies open into the ship's docking bay
```

### 1. Dual-Palm Forward Shove
- Detects when both controllers are pushed forward into the canopy boundary ($Z_{local} < -0.22\text{ m}$) with explosive forward velocity ($V_{z, left} < -0.28\text{ m/s} \land V_{z, right} < -0.28\text{ m/s}$).

### 2. Pneumatic Decompression Haptic Wave
- Delivers an $0.80$ amplitude pulse to both hands, simulating pressurized atmosphere escaping as the canopy is ejected.

## Consequences

### Positive
- Unforgettable opening immersion: physical breakout from cryo-stasis.
- Re-uses existing environmental dispatch pipeline without script injection.
