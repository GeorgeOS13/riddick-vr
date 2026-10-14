# ADR 0056: Inmate Shiv Boot Sheath Draw & Hidden Holster

## Status
Accepted / Implemented

## Context
In prison facilities like Butcher Bay, inmates conceal sharpened shivs, shanks, and razor scrapers inside leather work boots or beneath leg bindings to evade metal detectors and guard cavity searches. In VR, having physical access to an ankle/boot knife holster provides essential stealth immersion, allowing Riddick to inconspicuously draw or stow bladed weapons while crouching behind crates or sitting on inmate bunks.

## Decision
We implement boot sheath holster mechanics in Section 4 (Holstering & Inventory Gestures) of `src/hook/camera_hook.cpp`:
1. **Boot Sheath Spatial Zone**:
   - Controller positioned at the inside calf/ankle:
     $Y_{local} < -0.60\text{ m}$, $X_{local} \in [-0.25, 0.0]\text{ m}$ for left boot holster.
2. **Kinetic Draw Motion**:
   - Gripping with Left or Right hand ($> 0.50$) in the boot zone and drawing upward ($V_y > 0.35\text{ m/s}$) selects Slot '3' (Dual Ulaks / Melee Shiv) with a crisp $160\text{ Hz}$ ($0.45$ amplitude) leather sheath release click.
3. **Hidden Sheathing / Stowing**:
   - Releasing grip while holding a blade within the boot zone cleanly holsters the weapon, clearing hands for unarmed fist brawling.

## Consequences
- Complements the chest bandolier and waist hip holsters with a stealthy lower-body stowage location.
- Natural gesture when physically crouching or kneeling in shadows.
- Debounced ($800\text{ ms}$) to prevent accidental rapid weapon swapping.
