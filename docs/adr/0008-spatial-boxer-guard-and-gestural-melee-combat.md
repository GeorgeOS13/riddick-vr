# ADR 0008: Spatial Boxer Guard Detection, Melee Combat Disambiguation, and Tactical Weapon Reload Gestures

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*, combat transitions dynamically between visceral close-quarters bare-knuckle brawling (unarmed fists, improvised shivs, Ulaks) and tactical ranged gunplay (assault rifles, shotguns, sniper rifles, miniguns, SCAR gun).

### Reverse Engineering & Decompilation Findings
Reverse engineering of `DarkAthena.exe`, `Content\GameClasses_Win32_x86.dll`, `MSystem.dll`, `Content\Registry\RpgMelee.xrg`, `Content\Registry\RpgWeapons.xrg`, and user profile configs (`_profile`) revealed the underlying combat and weapon architecture:
1. **Melee Architecture (`CMeleeMapping`, `CRPG_Object_Melee`, `RpgMelee.xrg`):**
   - **Primary Attack (`ACTION_PRIMARYFIRE 240` / Left Mouse Button):** Executes directional melee strikes based on player movement vector + attack trigger:
     - Moving Forward + Attack -> Straight Punch / Thrust
     - Moving Backward + Attack -> Uppercut (`UpperCut01`)
     - Moving Left + Attack -> Left Hook / Slash (`COMBO_LEFT`)
     - Moving Right + Attack -> Right Hook / Slash (`COMBO_RIGHT`)
   - **Guard / Block State (`ACTION_ENTERFIGHTMODE` / `ANIM_BLOCK` / `MSG_ONBLOCK`):** Triggered by `ACTION_SECONDARYFIRE 241` (Right Mouse Button). When unarmed or holding melee weapons, holding Right Mouse Button causes Riddick to enter the guard stance, blocking incoming strikes and enabling counter-attacks (`UNARMED_COUNTER_...`, `CLUB_COUNTER_...`, `SHANK_COUNTER_...`).
2. **Weapon Reload System (`CRPG_Object_Weapon`, `RpgWeapons.xrg`):**
   - Triggered by `ACTION_RELOAD 19` (DirectInput scancode 0x13 / Key 'R').
   - Weapons specify `*reloadtime` (e.g. 1.5s - 2.0s), `*summon_ammoload`, `*summon_maxammo`, `*weapon_maxmagazines`, `*sound_reload`, and `*sound_reload_player`.
   - Pressing 'R' initiates `ANIM_RELOAD` in `AnimGraph2`, triggers audio signals, and refills the active magazine from reserve clips.

### VR Immersion Challenge
Previously, players relied solely on controller triggers or button taps. The user requested intuitive, responsive physical gestures:
1. **Physical Boxer Guard:** Raising both clenched fists up in front of the chin/face enters block mode in-game, without tracking flicker, jitter, or getting immobilized.
2. **Melee Disambiguation:** Punching gestures (jabs, hooks, uppercuts) must cleanly override and disengage block mode so punches are not blocked or canceled.
3. **Tactical Reload Gestures:** Physical interaction (bringing off-hand to the weapon receiver/mag-well, or reaching to the waist ammo pouch) must trigger reload consistently.

---

## Decision

We implemented a **Spatial Transformation and Hysteresis Debouncing System** in `src/hook/camera_hook.cpp`:

### 1. HMD-Relative Coordinate Transformation (`WorldToHmdLocal`)
Tracked controller positions from OpenXR are transformed into HMD local coordinates using the conjugate of the HMD orientation quaternion:
- $+X$: Player's right
- $+Y$: Player's up (eyes at $Y = 0$, chin/mouth at $Y \in [-0.25, -0.10]$)
- $-Z$: Player's forward gaze direction (in front of face at $Z \in [-0.40, -0.15]$)

This ensures guard poses and gestures function correctly regardless of the player's real-world room heading or head orientation.

### 2. Boxer Guard Pose Criteria
A guard state is detected when:
- **Chin / Eye Level:** Both controllers have $Y \in [-0.28\text{m}, +0.18\text{m}]$ relative to HMD.
- **In Front of Face:** Both controllers have $Z \in [-0.42\text{m}, -0.12\text{m}]$.
- **Close Hand Separation:** $|X_{\text{left}} - X_{\text{right}}| < 0.35\text{m}$.
- **Fists Clenched:** `leftController.grip > 0.40` and `rightController.grip > 0.40`.
- **Steady Hands:** Hand speeds $< 1.4\text{ m/s}$ (not mid-punch).
- **Outside Punch Cooldown:** $> 250\text{ms}$ since last executed punch.

### 3. Temporal Debouncing & Hysteresis
To prevent tracking jitter and false triggers:
- **Engagement Debounce:** Guard pose must hold continuously for $\ge 60\text{ms}$ before Right Mouse Down (`MOUSEEVENTF_RIGHTDOWN`) is dispatched.
- **Punch Instant Release:** If controller speed spikes ($> 1.6\text{ m/s}$), block is instantly released with $0\text{ms}$ delay.
- **Drop Smoothing:** When hands drop naturally, an $80\text{ms}$ release countdown prevents transient occlusion from flickering the block.
- **Haptic Confirmation:** Both controllers receive tactile haptic pulses ($0.35$ amplitude) when block engages.

```
       [Tracked Controllers in Front of Face]
                         │
                         ▼
           Guard Pose Criteria Met?
                 │               │
               Yes               No
                 │               │
        Held >= 60ms?       Punch Thrown?
           │        │        │        │
          Yes       No      Yes       No
           │        │        │        │
           ▼        ▼        ▼        ▼
       [ENGAGE]   [WAIT]  [RELEASE] Hand Dropped > 80ms?
       RightDown   Hold   RightUp        │          │
       Haptics             (0ms)        Yes         No
                                         │          │
                                         ▼          ▼
                                     [RELEASE]   [HOLD]
                                      RightUp    Sustain
```

### 4. Melee Punching Integration
When a physical strike is thrown (controller velocity $> 1.8\text{ m/s}$):
- Active block state is immediately disengaged (`MOUSEEVENTF_RIGHTUP`).
- Directional movement key ('W' for jab, 'A' for left hook, 'D' for right hook, 'S' for uppercut) is pressed.
- Primary fire (`MOUSEEVENTF_LEFTDOWN` / `MOUSEEVENTF_LEFTUP`) is pulsed.
- `s_lastPunchTime` is updated, suppressing guard pose re-entry for $250\text{ms}$ while the punch completes and returns.

### 5. Tactical Weapon Reload Gestures
Physical reloads are triggered via two natural motions:
1. **Receiver / Mag-Well Interaction:** Bringing off-hand within $20\text{cm}$ of the weapon hand with grip $> 0.35$.
2. **Waist Ammo Pouch:** Reaching down to hip level ($Y < -0.38\text{m}$ relative to HMD) with grip $> 0.38$.
- Evaluated only when not in boxer guard pose.
- 1100ms cooldown debounce prevents duplicate reloads.
- Dispatches `SendKeyTap('R')`, triggering `ACTION_RELOAD` in the game engine with distinctive dual haptic confirmation ($0.55$ off-hand, $0.75$ dominant hand).

### 6. Adaptive Physical Crouch & Dynamic Height Calibration (ADR 0004 & 0005)
Replaces hardcoded crouch offsets with dynamic player eye height calibration:
- **Standing Height Tracking:** On startup and standing upright, the system tracks peak natural eye height (`s_standingEyeHeight`).
- **Recenter Reset:** Whenever HMD recentering is triggered (e.g. Right Stick Click or IPC reset flag), `s_standingEyeHeight` recalibrates to the active headset position.
- **Engagement Threshold:** Physical crouch engages when the player ducks $> 28\text{cm}$ below their calibrated standing height (or drops below $78\%$ standing height).
- **Hysteresis Buffer:** Crouch remains held until rising within $18\text{cm}$ of standing height, completely eliminating boundary jitter while crouch-walking or ducking behind cover.

### 7. Two-Handed Firearm Aim Stabilization (ADR 0005)
Detects two-handed weapon grip to steady long-range aiming:
- **Grip Proximity:** Distance between controller grip poses $< 25\text{cm}$.
- **Support Hand Grip:** Off-hand grip button held (`leftController.grip > 0.45`).
- **Disambiguation:** Active only when not in boxer guard and not reloading.
- **Damping & Filtering:**
  - In-game look/aim delta is attenuated with a $0.65\times$ tremor damping factor.
  - Spherical linear interpolation (SLERP) and exponential smoothing ($\alpha = 0.35$) are applied to the weapon aim pose.
  - Off-hand 'E' interact input is suppressed while steadying the weapon.
  - Delivers subtle dual haptic feedback ($0.25$ amplitude) upon engaging.

### 8. Decoupled Motion Controller Aim Steering
To bridge the gap between traditional screen-centered mouse aiming and VR 6DOF motion controller weapon pointing:
- **Relative Coordinate Mapping:**
  The relative orientation between the HMD and weapon aim controller is calculated:
  $$Q_{rel} = Q_{hmd}^{-1} \times Q_{ctrl}$$
  The forward gaze vector $(0, 0, -1)$ rotated by $Q_{rel}$ yields the controller line-of-sight in HMD view space:
  $$fwdX = -2(w_{rel} y_{rel} + x_{rel} z_{rel})$$
  $$fwdY = 2(w_{rel} x_{rel} - y_{rel} z_{rel})$$
  $$fwdZ = -(1 - 2(x_{rel}^2 + y_{rel}^2))$$
  The relative horizontal and vertical aim angles are extracted:
  $$\theta_{yaw} = \text{atan2}(2(w_{rel} y_{rel} + z_{rel} x_{rel}), 1 - 2(x_{rel}^2 + y_{rel}^2))$$
  $$\theta_{pitch} = \text{asin}(\text{clamp}(fwdY, -1, 1))$$
- **Aim Steering & Elastic Center Pull:**
  The frame-to-frame controller angular velocity ($\Delta_{ctrl}$) and angular displacement from center ($\theta$) are blended into mouse look:
  $$\text{combinedYaw} = \Delta_{headYaw} + \Delta_{ctrlYaw} \cdot W_{aim} + \theta_{yaw} \cdot W_{pull}$$
  $$\text{combinedPitch} = \Delta_{headPitch} + \Delta_{ctrlPitch} \cdot W_{aim} + \theta_{pitch} \cdot W_{pull}$$
- **Dynamic Weighting & Disambiguation:**
  - **ADS / Two-Handed Stabilization:** $W_{aim} = 0.75$, $W_{pull} = 0.08$. High-precision aiming pulls the crosshair directly along the weapon barrel.
  - **Hip Fire:** $W_{aim} = 0.35$, $W_{pull} = 0.03$. Subtle steering allowing loose natural weapon pointing.
  - **Center Deadzone:** A $\pm 2.0^\circ$ ($0.035\text{ rad}$) deadzone prevents slow camera creeping when weapons are rested forward.
  - **Combat State Suppression:** When boxer guard (`s_physicalBlockEngaged`) or tactical reload gestures (`isReloading`) are active, $W_{aim}$ and $W_{pull}$ are completely zeroed ($0.0$), preventing erratic crosshair deflection while defending or reloading.

---

## Consequences

### Positive
- **Universal Ergonomics:** Adaptive crouch accommodates tall players, seated play, and differing play spaces without manual configuration.
- **Precision Aiming:** Two-handed rifle and sniper aiming removes micro-tremors and tracking noise.
- **Intuitive VR Gunplay:** Decoupled controller aim steering allows natural independent hand aiming, smoothly pulling the crosshair to the weapon's physical line-of-sight.
- **Rock-Solid Guard Stance:** Raising fists enters block mode consistently without stutter or accidental immobilization.
- **Fluid Combat Flow:** Boxer guard transitions seamlessly into jabs, hooks, and uppercuts with zero input latency.
- **Natural Firearms Handling:** Bringing hands together or grabbing at the waist reloads weapons intuitively without looking for controller buttons.
- **Full Backward Compatibility:** Left trigger (aim/block) and right grip (reload) remain active as explicit fallbacks.

### Challenges & Engine Quirks Identified
- In Starbreeze's `GameClasses_Win32_x86.dll`, holding `ACTION_SECONDARYFIRE` while executing `ACTION_PRIMARYFIRE` triggers a block counter if an opponent is attacking, but suppresses standard punches if no opponent strike is active. Explicitly lifting the right mouse button prior to firing a punch gesture resolves this animation priority conflict.
