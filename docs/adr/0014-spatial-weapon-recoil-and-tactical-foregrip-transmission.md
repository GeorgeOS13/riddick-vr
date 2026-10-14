# ADR 0014: Spatial Weapon Recoil Displacement and Foregrip Haptic Transmission

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena*, firing firearms (such as the pump-action shotgun, assault rifle, and pistol) induces aggressive animation-driven screen shake and vertical weapon climb.

In virtual reality:
- Shaking or pitching the player's primary view camera during weapon fire causes immediate vestibular dissonance and ocular fatigue.
- Conversely, eliminating recoil entirely renders gunplay sterile and weightless.
- Modern high-fidelity VR shooters (e.g. *Half-Life: Alyx*, *Boneworks*) isolate recoil kinematics strictly to the weapon's barrel vector and motion controller haptics, leaving the head-tracked world horizon unaffected.
- Furthermore, when the player supports a firearm with both hands (ADR 0005 Two-Handed Stabilization), mechanical recoil should physically dampen muzzle rise and transmit vibrations across both hands.

## Decision

We implement **Kinematic Barrel Recoil & Dual-Hand Foregrip Transmission** in `src/hook/camera_hook.cpp`:

```
                 [Right Trigger Depressed: Fire Event]
                                   │
         ┌─────────────────────────┴─────────────────────────┐
         ▼                                                   ▼
 [One-Handed Hip Fire]                     [Two-Handed ADS Stabilization]
 - Muzzle Kick: +0.040 rad (~2.3°)         - Muzzle Kick: +0.022 rad (~1.3°) (45% Dampened)
 - Right Haptic: 0.82 (180 Hz)             - Right Haptic: 0.65 (180 Hz)
 - Left Haptic: 0.00                       - Left Foregrip Haptic: 0.45 (120 Hz)
         │                                                   │
         └─────────────────────────┬─────────────────────────┘
                                   ▼
                  [Exponential Recovery: * 0.82 / frame]
                  tau ~ 120 ms -> Zero residual drift
```

### 1. Kinematic Muzzle Climb Injection
- When primary fire is initiated (`wantRightTrigger && !s_rightTriggerDown`), a transient upward pitch offset (`s_recoilKickPitch`) is injected directly into controller aim steering (`ctrlPitch += s_recoilKickPitch`).
- **One-Handed:** $+0.040\text{ rad}$ ($\approx 2.3^\circ$).
- **Two-Handed:** $+0.022\text{ rad}$ ($\approx 1.3^\circ$, dampened by $45\%$).
- **Exponential Recovery:** Recoil decays smoothly via `s_recoilKickPitch *= 0.82f` every frame, restoring the exact rest aiming vector over $\approx 120\text{ ms}$ without lingering drift.

### 2. Dual-Hand Foregrip Haptic Transmission
- **Primary Hand Actuator:** Firing emits a $0.82$ impulse (one-handed) or $0.65$ impulse (two-handed) at $180\text{ Hz}$ through the firing hand.
- **Off-Hand Foregrip Transmission:** When two-handed stabilization is active (`s_twoHandedStabilizationActive`), mechanical energy propagates along the barrel, emitting a secondary $0.45$ vibration at $120\text{ Hz}$ through the supporting forward grip hand.

## Consequences

### Positive
- Rock-solid head tracking: eliminates motion sickness by keeping the world horizon completely free of camera shake.
- Visceral tactile gunplay: realistic muzzle rise and mechanical impulse tailored to weapon handling style.
- Incentivizes realistic two-handed firearm handling with tangible stability advantages.
