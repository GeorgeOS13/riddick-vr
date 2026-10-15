# ADR 0246: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Kinetic Cleave Shockwave

## Status
Accepted

## Context
Engaging entrenched armored riot guards behind defensive ramparts requires an airborne vault combined with an overhead two-handed kinetic cleave, channeling gravity and arc energy directly into targets upon impact.

## Decision
Implement physical stun baton vault jump kinetic cleave shockwave mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), raise energized baton overhead in two-handed grip (`handsDistSledge < 0.28\text{ m}`, `rightLocal.y > 0.15\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Explosive Downward Cleave on Descent**: Bring both controllers down with extreme downward velocity (`rVel.y < -1.45\text{ m/s} \land lVel.y < -1.15\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `S` + `LMB` for airborne leap, forward momentum, arc overcharge, plant posture, and concussive cleaving impact.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) kinetic plasma detonation blast on the right controller and $80\text{ Hz}$ ($0.90$) two-handed landing frame recoil on the left controller.

## Consequences
- Devastating airborne anti-armor finisher against high-value security targets.
- Complements vault ground arc rings, kinetic cleaves, and shield bashes.
