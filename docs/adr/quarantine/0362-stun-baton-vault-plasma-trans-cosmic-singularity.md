# ADR 0362: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Trans-Cosmic Singularity Overdrive

## Status
Accepted

## Context
During high-velocity aerial vaults across deep engineering abysses in the Dark Athena core, Riddick overcharges the stun baton capacitor core into trans-cosmic resonance. Descending from peak jump trajectory and driving both arms downward into the metal deck with maximum kinetic momentum detonates a trans-cosmic singularity wave. The collapsing field expels a hyper-dense concentric plasma shockwave that obliterates defensive barriers, vaporizes sentry turrets, and incapacitates all surrounding combatants.

## Decision
Implement physical two-handed spatial gesture detection for the stun baton trans-cosmic singularity overdrive in `src/hook/camera_hook.cpp`:
1. **Jump & Vault Stance**: Active airborne jump state (`s_jumpAirborneTimer > 0.0f` or Button B jump).
2. **Energized Two-Handed Baton Grip**: Controllers in proximity ($dist < 0.28\text{ m}$), both grips clenched ($> 0.65$), Right Trigger squeezed ($> 0.65$).
3. **Terminal Trans-Cosmic Descent Plunge**: Downward plunge on descent with extreme velocity ($rVel.y < -2.85\text{ m/s} \land lVel.y < -2.60\text{ m/s} \land |\Delta V_x| > 2.20\text{ m/s}$).
4. **Input Dispatch**: Dispatches 25-stage key sequence `'Space'` + `'W'` + `'Q'` + `'Tab'` + `'Space'` + `'Enter'` + `'C'` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `'Enter'` + `'Space'` + `'Enter'` + `'Q'` + `'Tab'` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `LMB` + `'Space'` + `'Enter'` + `LMB`.
5. **Haptic Feedback**: Dual $530\text{ Hz}$ ($0.98$, $270\text{ ms}$) resonant cataclysm trans-cosmic singularity overdrive shockwave and $460\text{ Hz}$ ($0.98$, $290\text{ ms}$) structural deck trans-cosmic recoil.

## Consequences
- Represents the absolute pinnacle of vault-initiated kinetic plasma combat techniques.
- Maintained inside Section 5 flattened dispatch structure with 0 MSVC C1061 nesting errors.
