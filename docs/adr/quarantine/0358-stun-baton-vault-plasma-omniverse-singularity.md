# ADR 0358: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Omniverse Singularity Overdrive

## Status
Accepted

## Context
When performing aerial vaults across deep abyss shafts or over reinforced defense barricades in the Dark Athena core reactor corridors, Riddick channels multi-stage capacitive resonant power into the stun baton tip. Descending from peak jump velocity and driving both arms downward into the deck plate triggers an omniverse singularity collapse. The shockwave cascades ionized plasma across a wide radius, overwhelming enemy force fields, destroying drones, and neutralizing riot guards.

## Decision
Implement physical two-handed spatial gesture detection for the stun baton omniverse singularity overdrive in `src/hook/camera_hook.cpp`:
1. **Jump & Vault Stance**: Active airborne jump state (`s_jumpAirborneTimer > 0.0f` or Button B jump).
2. **Energized Two-Handed Baton Grip**: Controllers in proximity ($dist < 0.28\text{ m}$), both grips clenched ($> 0.65$), Right Trigger squeezed ($> 0.65$).
3. **Terminal Omniverse Descent Plunge**: Downward plunge on descent with extreme velocity ($rVel.y < -2.80\text{ m/s} \land lVel.y < -2.55\text{ m/s} \land |\Delta V_x| > 2.15\text{ m/s}$).
4. **Input Dispatch**: Dispatches 24-stage key chord `'Space'` + `'W'` + `'Q'` + `'Tab'` + `'Space'` + `'Enter'` + `'C'` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `'Enter'` + `'Space'` + `'Enter'` + `'Q'` + `'Tab'` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `LMB`.
5. **Haptic Feedback**: Dual $520\text{ Hz}$ ($0.98$, $260\text{ ms}$) resonant cataclysm omniverse singularity overdrive shockwave and $450\text{ Hz}$ ($0.98$, $280\text{ ms}$) structural deck omniverse recoil.

## Consequences
- Represents the zenith of vault-initiated kinetic plasma combat techniques.
- Maintained inside Section 5 flattened dispatch structure with 0 MSVC C1061 nesting errors.
