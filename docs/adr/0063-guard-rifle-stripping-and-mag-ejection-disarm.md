# ADR 0063: Guard Rifle Firearm Stripping & Mag Ejection Disarm

## Status
Accepted / Implemented

## Context
In *The Chronicles of Riddick*, close-quarters combat against armed mercenary assault rifle guards allows Riddick to counter incoming muzzle strikes with lightning-fast disarms. Starbreeze's original animations featured Riddick snatching the rifle barrel, stripping the magazine, and turning the weapon back against the guard.

In virtual reality, executing this disarm physically requires a decisive two-handed kinetic intercept with deliberate trigger engagement, capturing the tactile violence of wrenching a hot rifle receiver from an adversary's grip.

## Decision
We implement firearm stripping and magazine ejection disarms in Section 5a of `src/hook/camera_hook.cpp`:
1. **Boxer Guard Counter Intercept**:
   - Player maintains Boxer Guard defense (`s_currentBlockEngaged`).
   - Thrusts both hands forward into the incoming weapon:
     $V_{z, right} < -1.15\text{ m/s} \land V_{z, left} < -1.15\text{ m/s}$.
2. **Dual-Trigger Stripping Engagement**:
   - Both controller triggers squeezed ($> 0.60$) during the forward thrust to grasp the receiver and depress the magazine release catch.
3. **Disarm Dispatch & Haptics**:
   - Dispatches Counter-Disarm attack (`LMB` + `'R'` mag drop + `'1'` firearm equip).
   - Delivers a bone-jarring $0.95$ amplitude ($280\text{ Hz}$) metallic wrenching and shearing shockwave across both controllers.

## Consequences
- Elevates melee counter-combat against firearms to a physical room-scale feat.
- Dual-trigger requirement prevents accidental activation during standard double-palm shoves.
- Seamlessly equips the stripped firearm ready for immediate point-blank retaliatory fire.
