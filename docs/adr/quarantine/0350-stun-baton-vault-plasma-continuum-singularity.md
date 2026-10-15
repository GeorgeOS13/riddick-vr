# ADR 0350: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Continuum Singularity Overdrive

## Status
Accepted

## Context
When vaulting forward across high-voltage conduits, security barriers, or catwalk gaps in the Dark Athena sub-decks with a stolen security stun baton, Riddick channels maximum stored electrostatic plasma into the baton core during freefall. Building upon the interdimensional vault overdrive (ADR 0346), plunging downward into the deck plate upon touchdown detonates an omnidirectional continuum singularity shockwave that collapses hostile riot shields and stuns armored sentries across the entire blast perimeter.

## Decision
Implement physical two-handed spatial gesture detection for the stun baton continuum singularity overdrive in `src/hook/camera_hook.cpp`:
1. **Jump & Vault Stance**: Active airborne jump state (`s_jumpAirborneTimer > 0.0f` or Button B jump).
2. **Energized Two-Handed Baton Grip**: Controllers in proximity ($dist < 0.28\text{ m}$), both grips clenched ($> 0.65$), Right Trigger squeezed ($> 0.65$).
3. **Terminal Continuum Descent Plunge**: Downward plunge on descent with extreme velocity ($rVel.y < -2.70\text{ m/s} \land lVel.y < -2.45\text{ m/s} \land |\Delta V_x| > 2.05\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'Space'` + `'W'` + `'Q'` + `'Tab'` + `'Space'` + `'Enter'` + `'C'` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `'Enter'` + `'Space'` + `'Enter'` + `'Q'` + `'Tab'` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `LMB`.
5. **Haptic Feedback**: Dual $500\text{ Hz}$ ($0.98$, $260\text{ ms}$) resonant cataclysm continuum singularity overdrive shockwave and $430\text{ Hz}$ ($0.98$, $280\text{ ms}$) structural deck continuum recoil.

## Consequences
- Produces an ultimate high-voltage kinetic weapon technique for vertical arena traversal and boss engagements.
- Maintained within Section 5 flattened dispatch structure.
