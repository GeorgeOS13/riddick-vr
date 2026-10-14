# ADR 0035: Physical Shiv Sharpening and Blade Grinding Gestures

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay*:
1. **Prison Improvised Weaponry:** Riddick crafts improvised lethal shivs out of screwdrivers, sharpened steel rods, dining room cutlery, and glass fragments.
2. **Blade Maintenance & Hone:** Keeping blades razor-sharp for lethal throat slits and counter-disarms is a core prison survival ritual.
3. **VR Physicality:** In VR, players expect to physically draw a whetstone or second blade across the edge in reciprocal strokes to hone the cutting surface.

## Decision

We implement **Reciprocal Blade Honing & Sharpening Detection** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
             [Melee Weapon / Shiv Equipped in Dominant Hand]
             [Off-Hand Brought into Edge Contact: Dist < 14cm]
                                   │
                                   ▼
             [Reciprocating Stroke along Blade: |Delta Y_rel| > 8cm]
                                   │
                                   ▼
             [Blade Sharpening Stroke Executed]
             - Injects tactile metallic scrape pulse: 0.55 Amplitude (220 Hz)
             - Hones blade edge for guaranteed critical strike damage
```

### 1. Close Blade Contact Proximity
- Evaluates when both hands are close together in front of the chest ($dist < 14\text{ cm}$) with grips clenched ($> 0.40$).

### 2. Reciprocal Longitudinal Stroke Kinematics
- Measures the relative velocity and displacement along the blade axis ($|\Delta Y_{rel}| > 0.08\text{ m}$).
- Delivers a crisp metallic rasping vibration at $220\text{ Hz}$ ($0.55$ amplitude) on each stroke.

## Consequences

### Positive
- Deepens prison survival roleplay and visceral immersion in Butcher Bay.
- Satisfying physical tactile feedback preparing for stealth infiltrations.
