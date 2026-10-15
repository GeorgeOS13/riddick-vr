# ADR 0250: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Concussive Deck Impact Shockwave

## Status
Accepted

## Context
Clearing tight security checkpoints fortified with heavy floor deployable barriers requires vaulting over the obstacle and detonating the energized stun baton directly into the floor on descent, generating a hemispherical electro-kinetic blast that shatters barrier anchors and stuns guards.

## Decision
Implement physical stun baton vault jump concussive deck impact shockwave mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Plunge**: Drive both controllers violently straight down into the floor on descent (`rVel.y < -1.50\text{ m/s} \land lVel.y < -1.25\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Space` + `LMB` for airborne jump, forward trajectory, arc activation, shockwave compression, and deck impact blast.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) concussive electro-kinetic floor blast on both controllers and $60\text{ Hz}$ ($0.98$) structural deck resonance.

## Consequences
- Massive AOE crowd-control strike from jump or vault.
- Integrates seamlessly with vault kinetic cleaves, vault ground arcs, and grounding thrusts.
