# ADR 0306: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Hypernova

## Status
Accepted

## Context
In apex-level security combat breaches against dreadnought mechs and fortified riot phalanxes, discharging both overcharged stun batons during a high-arc airborne leap and slamming both tips with hyper-kinetic impact into deck plating triggers a resonant cataclysm hypernova, releasing an omnidirectional kinetic-plasma shockwave that vaporizes physical structures and fries all cybernetic circuits in a massive radius.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm hypernova mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Hypernova Expansion**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -2.15\text{ m/s} \land lVel.y < -1.90\text{ m/s} \land |\Delta V_x| > 1.50\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` + `LMB` + `Space` for airborne leap, forward drive, capacitor overload, hypernova expansion, deck compression, and omnidirectional hyper-resonance detonation.
- **Haptic Feedback**: $400\text{ Hz}$ ($0.98$) resonant cataclysm hypernova shockwave on right controller and $260\text{ Hz}$ ($0.98$) structural deck hypernova recoil on left controller.

## Consequences
- The absolute highest-echelon high-voltage AOE combat finisher.
- Delivers catastrophic destructive power against dense clusters of armored adversaries.
