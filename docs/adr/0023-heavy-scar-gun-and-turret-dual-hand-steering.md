# ADR 0023: Heavy SCAR Gun Mine Detonation, Low-Spine Holstering, and Mounted Turret Dual-Hand Steering

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*, heavy weapon combat introduces two iconic systems:
1. **SCAR Rifle (Sub-munitions Compressed Air Rifle):**
   - Fires up to 5 air-compressed darts that latch onto enemy armor, drones, or architectural surfaces.
   - Secondary Fire triggers simultaneous detonator charges, obliterating targets in a combined shockwave.
2. **Mounted Security Turrets & Mech Gatlings:**
   - Dual spade-grip heavy emplacements delivering sustained suppression fire.
3. **Heavy Weapon Holstering:**
   - In addition to primary rifles, sidearms, and blades, heavy ordinance is mounted on Riddick's lower magnetic back harness.

In flat desktop controls:
- Selecting heavy weapons required cycling through keys or weapon menus.
- Detonating SCAR mines required pressing `Right Mouse Button`, which conflicted with aim down sights.

## Decision

We implement **Spatial Heavy Ordinance Dynamics** in Section 6c and 6f of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Right Controller at Low Spine / Lower Back]
                   (Y < -0.30m, Z > -0.05m, |X| < 0.15m, Grip > 0.50)
                                        │
                                        ▼
             [Equips Slot '5': Heavy SCAR Rifle / Mounted Minigun]
             - Heavy Magnetic Clamp Haptic (0.85)
                                        │
                                        ▼
              [Two-Handed Firearm Stabilization Active (< 25cm)]
                                        │
                                        ▼
             [Off-Hand Clench: Left Trigger > 0.85 & Left Grip > 0.60]
                                        │
                                        ▼
             [Executes Secondary Fire: SCAR Air Mine Detonation]
             - Dispatches RMB Down / Up Pulse
             - Simultaneous Detonation of all Armed Charges
             - Thunderous Dual-Controller Concussion Wave (0.95, 280 Hz)
```

### 1. Low-Spine Heavy Weapon Holster
- Reaching behind the lower waist / lumbar spine ($Y_{local} < -0.30\text{ m}, Z_{local} > -0.05\text{ m}, |X_{local}| < 0.15\text{ m}$) with right grip held ($> 0.50$) equips weapon slot `5` (`SendKeyTap('5')`).
- Delivers a heavy $0.85$ magnetic clamp pulse ($180\text{ Hz}$).

### 2. Off-Hand Detonator Clench
- While holding the SCAR gun in two-handed stance, squeezing the off-hand trigger fully ($> 0.85$) while gripping ($> 0.60$) triggers the remote detonator.
- Transmits an explosive $0.95$ dual-haptic concussion shockwave ($280\text{ Hz}$), conveying the massive blast wave through both arms.

## Consequences

### Positive
- Intuitive spatial access to heavy ordnance from the lower back harness.
- Visceral two-handed tactile detonation of air-compressed mines without needing keyboard commands.
