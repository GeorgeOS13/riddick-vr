# ADR 0015: Spatial Weapon Holstering and Physical Inventory Gestures

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena*, Riddick seamlessly alternates between three combat categories:
1. **Primary Long-Arms:** Assault rifles, shotguns, tranq rifles, and heavy cannons (`Key 1`).
2. **Sidearms & Concealed Blades:** Semi-automatic pistols, tranquilizers, and screwdrivers/shivs (`Key 2`).
3. **Dedicated Melee Weapons:** Dual curved Ulaks, brass knuckles, and bare fists (`Key 3` / `Key 4`).

In flat gaming, inventory switching was performed via number keys or cycling the gamepad D-Pad. In VR, pressing buttons to cycle weapons breaks physical presence. Players naturally expect to reach over their shoulder to whip out a shotgun, draw a sidearm from their right hip, or unsheathe dual Ulaks from their left belt harness.

## Decision

We implement a **Spatial Kinematic Holster Engine** in `src/hook/camera_hook.cpp`:

```
                 [Player Controller Poses Relative to HMD]
                                     │
         ┌───────────────────────────┼───────────────────────────┐
         ▼                           ▼                           ▼
[Right Over-Shoulder]         [Right Hip Holster]         [Left Hip Holster]
- Pos: Y > +0.15m, Z > -0.05m - Pos: Y < -0.32m, X > +0.14m - Pos: Y < -0.32m, X < -0.14m
- Grip squeezed (> 0.50f)     - Grip squeezed (> 0.50f)   - Grip squeezed (> 0.50f)
- Primary Long Weapon ('1')   - Sidearm / Pistol ('2')    - Melee / Dual Ulaks ('3')
- Mechanical latch (0.75)     - Leather holster (0.65)    - Metallic blade draw (0.70)
```

### 1. Spatial Holster Zones
Each controller's position is evaluated relative to the player's HMD space:

- **Over-the-Shoulder Primary Weapon Holster:**
  Reaching back over the right shoulder ($Y_{local} > 0.15\text{ m}, Z_{local} > -0.05\text{ m}$) with grip squeezed ($> 0.50$) toggles the Primary Weapon slot (`SendKeyTap('1')`), delivering a solid mechanical latch haptic pulse ($0.75$).
- **Right Hip Sidearm Holster:**
  Reaching down to the right hip ($Y_{local} < -0.32\text{ m}, X_{local} > 0.14\text{ m}$) with grip squeezed ($> 0.50$) toggles the Sidearm / Pistol slot (`SendKeyTap('2')`), delivering a crisp friction draw pulse ($0.65$).
- **Left Hip Melee / Ulaks Holster:**
  Reaching down to the left hip ($Y_{local} < -0.32\text{ m}, X_{local} < -0.14\text{ m}$) with grip squeezed ($> 0.50$) toggles the Melee / Dual Ulaks slot (`SendKeyTap('3')`), delivering a metallic blade unsheathe pulse ($0.70$).

### 2. State Isolation and Hysteresis
- **Debounce:** A $900\text{ ms}$ temporal gate prevents rapid double-swaps.
- **Suppression:** Holster gestures are suppressed during ladder climbing (`isClimbingActive`), boxer guard (`s_physicalBlockEngaged`), and two-handed firearm stabilization (`s_twoHandedStabilizationActive`).

## Consequences

### Positive
- Fluid, visceral physical weapon handling matching contemporary VR benchmark shooters.
- Differentiated tactile sensations distinguishing drawing heavy firearms from unholstering pistols and curved blades.
- Zero reliance on gamepad button cycling or artificial inventory radial menus during combat.
