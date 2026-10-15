# ADR 0376: Weapon Slot Recoil & Haptic Profile Calibration

## Status
Accepted

## Context
When firing weapons in `src/hook/camera_hook.cpp`, tactile haptic feedback and angular muzzle climb were previously driven by a single static constant ($0.82$ amplitude and $0.040\text{ rad}$ pitch rise, damped to $0.65$ and $0.022\text{ rad}$ when two-handed).

This generic feedback failed to convey the distinct physical characteristics of the game's diverse arsenal:
- Melee weapons (fists, shivs, Ulaks) received firearm-like muzzle climbs.
- Heavy automatic weapons (rotary chainguns, SCAR rifle) lacked progressive recoil impulse accumulation.
- Shotguns lacked the concussive punch and foregrip vibration expected of a 12-gauge breach blast.
- Precision sidearms (semi-automatic pistols, tranquilizer guns) had excessive muzzle displacement.

## Decision
Implement weapon slot tracking and dynamic firearm recoil profiling in `src/hook/camera_hook.cpp`:

1. **Active Slot State Tracking**:
   - Track `s_activeWeaponSlot` across holster draws, quick-select inputs, and physical inventory reaches:
     - Slot 1: Primary firearm (Assault rifle / shotgun / carbine)
     - Slot 2: Sidearm (Pistol / tranquilizer gun)
     - Slot 3: Melee / blades (Fists / shivs / Ulaks)
     - Slot 4: Tactical ordnance (Frag grenades)
     - Slot 5: Heavy weapon (SCAR rifle / rotary chaingun)

2. **Calibrated Kinetic Impulse Matrix**:
   - **Slot 3 (Melee)**: Zero muzzle pitch climb ($\theta = 0$), crisp high-frequency slash impulse ($0.45\text{ amplitude}$).
   - **Slot 2 (Sidearm / Tranq)**: Moderate kick ($\theta = 0.024\text{ rad}$), sharp tactile snap ($0.60\text{ amplitude}$).
   - **Slot 1 (Riot Shotgun / Long-Arm)**: High concussive blast ($\theta = 0.055\text{ rad}$ single-handed, $0.028\text{ rad}$ two-handed), maximum dual-hand impulse ($0.95$ right, $0.65$ left foregrip).
   - **Slot 4 (Assault Rifle)**: Fast cyclic impulse ($\theta = 0.020\text{ rad}$, $0.70\text{ amplitude}$).
   - **Slot 5 (Heavy SCAR / Minigun)**: Sustained concussive kick ($\theta = 0.065\text{ rad}$ single-handed, $0.025\text{ rad}$ two-handed braced, $0.98$ dominant, $0.75$ chassis counter-torque).

3. **Spatial Recoil Decay**:
   - Exponential recovery rate tuned to $120\text{ ms}$ half-life ($\times 0.82$), returning sight picture cleanly to target bore axis.

## Consequences
- Every weapon class produces tactile, distinct physical kick profiles matching its visual and audio design.
- Greatly enhances immersion during intense firefights on the Dark Athena.
