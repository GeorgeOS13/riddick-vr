# ADR 0024: Heavy Mech Exoskeleton Sustained Rotary Fire and Cyclic Feedback Dynamics

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena*, Riddick pilots heavy combat mechs / Heavy Guard exoskeletons and wields rapid-fire rotary miniguns and submachine guns:
1. **High-Rate Sustained Fire:** Weapons like the rotary chaingun and Heavy Guard minigun deliver hundreds of rounds per minute with high muzzle climb and mechanical vibration.
2. **Tactile Falloff Problem:** In earlier VR implementations, a trigger press generated only a single initial recoil impulse, leaving subsequent automatic fire completely silent/numb in the player's hands.
3. **Foregrip Transmission:** In heavy weapon stances, the off-hand supporting the weapon frame should experience proportional mechanical vibration.

## Decision

We implement a **Cyclic Rotary Haptic Engine** in Section 8 of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Right Trigger Held (> 0.5f)]
                                 │
                                 ▼
                    [Hold Duration > 150ms]
                                 │
                                 ▼
             [Every 90ms: Emit Cyclic Rotary Impulse]
             - Dominant Hand: 0.68 Amplitude (180 Hz)
             - If Two-Handed: Foregrip Transmission 0.42 Amplitude (120 Hz)
             - Simulates high-speed motor spin and mechanical bolt reciprocation
```

### 1. Temporal Hold Detection
- Tracks the continuous duration of `wantRightTrigger`.
- After an initial $150\text{ ms}$ engagement threshold, enters cyclic automatic mode.

### 2. Multi-Band Cyclic Pulses
- Injects a $0.68$ amplitude pulse every $90\text{ ms}$ into the firing controller.
- When two-handed stabilization is active (`s_twoHandedStabilizationActive`), transmits a $0.42$ dampening pulse to the off-hand foregrip, conveying the thunderous mechanical resonance of continuous heavy fire.

## Consequences

### Positive
- Heavy mechs, rotary gatlings, and automatic rifles feel immensely powerful and physically alive.
- Natural tactile indication of active sustained fire without looking at ammo counters.
