# ADR 0310: Security Guard Stun Baton Forward High-Voltage Vault Jump Thrust & Omnidirectional Kinetic Plasma Resonant Cataclysm Singularity Overdrive

## Status
Accepted

## Context
In maximum-security breaches against colossal combat mechs and fortified riot phalanxes, discharging both overcharged stun batons during a high-arc airborne vault jump and driving both tips with full gravitational acceleration into deck plating generates a kinetic plasma resonant cataclysm singularity overdrive, collapsing surrounding energy shields into an implosive vortex before detonating an omnidirectional shockwave that obliterates all nearby hostiles.

## Decision
Implement physical stun baton vault jump omnidirectional kinetic plasma resonant cataclysm singularity overdrive mechanics in `src/hook/camera_hook.cpp`:
- **Airborne Stance**: During an airborne vertical jump or controller jump (`(vrState.rightController.buttons & RIDDICK_VR_BTN_B) != 0 || rVel.y > 0.85f || lVel.y > 0.85f`), maintain two-handed energized baton grip (`handsDistSledge < 0.28\text{ m}`, both grips $> 0.65$, right trigger $> 0.65$).
- **Violent Two-Handed Floor Cleave with Singularity Overdrive**: Plunge both controllers downward with maximum velocity on descent (`rVel.y < -2.20\text{ m/s} \land lVel.y < -1.95\text{ m/s} \land |\Delta V_x| > 1.55\text{ m/s}`).
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `Tab` + `Space` + `Enter` + `C` + `Space` + `Enter` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Q` + `Tab` + `Space` + `Enter` + `LMB` + `Space` + `Enter` + `Tab` for airborne leap, forward drive, capacitor overload, singularity vortex creation, deck compression, and catastrophic hyper-resonance detonation.
- **Haptic Feedback**: $410\text{ Hz}$ ($0.98$) resonant cataclysm singularity overdrive shockwave on right controller and $280\text{ Hz}$ ($0.98$) structural deck singularity recoil on left controller.

## Consequences
- The absolute highest echelon of high-voltage kinetic AOE combat finishers.
- Obliterates armored mechanical hulls and shield generators across a massive combat perimeter.
