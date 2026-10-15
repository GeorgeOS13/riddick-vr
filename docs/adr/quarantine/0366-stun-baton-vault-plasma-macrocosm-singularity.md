# ADR 0366: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Macrocosm Singularity Overdrive

## Status
Accepted

## Context
When executing high-velocity aerial vaults across abyss chasms or reactor catwalks on the Dark Athena, Riddick charges the stun baton's capacitive matrix to macrocosm harmonic saturation. Driving both controllers downward into the deck plate upon touchdown triggers a catastrophic plasma implosion that unleashes an expanding macrocosmic plasma ring, neutralizing heavy security drones and armored riot squads across the chamber.

## Decision
Implement physical two-handed spatial gesture detection for the stun baton macrocosm singularity overdrive in `src/hook/camera_hook.cpp`:
1. **Jump & Vault Stance**: Active airborne jump state (`s_jumpAirborneTimer > 0.0f` or Button B jump).
2. **Energized Two-Handed Baton Grip**: Controllers in proximity ($dist < 0.28\text{ m}$), both grips clenched ($> 0.65$), Right Trigger squeezed ($> 0.65$).
3. **Terminal Macrocosm Descent Plunge**: Downward plunge on descent with extreme velocity ($rVel.y < -2.90\text{ m/s} \land lVel.y < -2.65\text{ m/s} \land |\Delta V_x| > 2.25\text{ m/s}$).
4. **Input Dispatch**: Dispatches 26-stage key sequence `'Space'` + `'W'` + `'Q'` + `'Tab'` + `'Space'` + `'Enter'` + `'C'` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `'Enter'` + `'Space'` + `'Enter'` + `'Q'` + `'Tab'` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `LMB` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `LMB`.
5. **Haptic Feedback**: Dual $540\text{ Hz}$ ($0.98$, $280\text{ ms}$) resonant cataclysm macrocosm singularity overdrive shockwave and $470\text{ Hz}$ ($0.98$, $300\text{ ms}$) structural deck macrocosm recoil.

## Consequences
- Represents an expanded tier of vault-initiated kinetic plasma combat techniques.
- Maintained inside Section 5 flattened dispatch structure with 0 MSVC C1061 nesting errors.
