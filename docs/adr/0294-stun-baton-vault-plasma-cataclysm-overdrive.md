# ADR 0294: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Overdrive

## Status
Accepted

## Context
When facing armored riot mech units and entrenched guard squadrons, overcharging dual stun batons beyond maximum safety thresholds during an airborne vault jump and driving both tips into structural deck plating unleashes an omnidirectional kinetic plasma resonant cataclysm overdrive that electrifies sub-floor conduits and obliterates surrounding enemies.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Cataclysmic Overdrive**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -2.00\text{ m/s} \land lVel.y < -1.75\text{ m/s} \land |\Delta V_x| > 1.35\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` for leap, forward surge, capacitor overdrive, plasma cataclysm propagation, girder resonance, and full-spectrum deck detonation.
- **Haptic Feedback**: $380\text{ Hz}$ ($0.98$) resonant cataclysm overdrive shockwave on right controller and $200\text{ Hz}$ ($0.98$) structural girder recoil on left controller.

## Consequences
- Pinnacle high-voltage AOE combat finisher.
- Naturally culminates the stun baton progression from kinetic pulse waves to resonant cataclysm overdrives.
