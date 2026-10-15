# ADR 0370: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Universal Singularity Overdrive

## Status
Accepted

## Context
When performing high-altitude leaps across multi-story reactor abysses or defense bastions on the Dark Athena, Riddick pushes the stun baton capacitor cells into universal harmonic saturation. Descending with terminal jump velocity and thrusting both arms into the metal floor plates triggers a universal singularity implosion. The collapsing plasma bubble violently detonates into an omnidirectional shockwave that shatters reinforced blast walls, disables mechanized walkers, and neutralizes all enemy forces within the sector.

## Decision
Implement physical two-handed spatial gesture detection for the stun baton universal singularity overdrive in `src/hook/camera_hook.cpp`:
1. **Jump & Vault Stance**: Active airborne jump state (`s_jumpAirborneTimer > 0.0f` or Button B jump).
2. **Energized Two-Handed Baton Grip**: Controllers in proximity ($dist < 0.28\text{ m}$), both grips clenched ($> 0.65$), Right Trigger squeezed ($> 0.65$).
3. **Terminal Universal Descent Plunge**: Downward plunge on descent with extreme velocity ($rVel.y < -2.95\text{ m/s} \land lVel.y < -2.70\text{ m/s} \land |\Delta V_x| > 2.30\text{ m/s}$).
4. **Input Dispatch**: Dispatches 27-stage key sequence `'Space'` + `'W'` + `'Q'` + `'Tab'` + `'Space'` + `'Enter'` + `'C'` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `'Enter'` + `'Space'` + `'Enter'` + `'Q'` + `'Tab'` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Space'` + `LMB` + `'Space'` + `'Enter'` + `'Tab'` + `LMB` + `'Space'` + `'Enter'` + `LMB` + `'Space'` + `LMB` + `'Space'` + `LMB`.
5. **Haptic Feedback**: Dual $550\text{ Hz}$ ($0.98$, $290\text{ ms}$) resonant cataclysm universal singularity overdrive shockwave and $480\text{ Hz}$ ($0.98$, $310\text{ ms}$) structural deck universal recoil.

## Consequences
- Elevates the stun baton's kinetic aerial plasma capabilities to universal status.
- Preserves single-level compound nesting in MSVC compilation.
