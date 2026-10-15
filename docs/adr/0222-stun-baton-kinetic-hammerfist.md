# ADR 0222: Security Guard Stun Baton Forward High-Arc Kinetic Hammerfist Deck Strike

## Status
Accepted

## Context
Breaking through reinforced armor or crushing grounded opponents requires channeling the momentum of an overhead swing into a heavy downward hammerfist strike driven through the energized stun baton chassis.

## Decision
Implement physical kinetic hammerfist strike in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Elevate dominant arm ($rightLocal.y > 0.15\text{ m}$, Right Grip $> 0.65$, Right Trigger $> 0.65$).
- **Violent Hammerfist Plunge**: Plunge dominant arm downward violently ($rVel.y < -1.35\text{ m/s}$) while stepping/driving forward ($rVel.z < -0.80\text{ m/s}$).
- **Synthesized Controls**: Emits `S` + `W` + `Q` + `LMB` for forward momentum, downward kinetic hammerfist, and electro-shock detonation.
- **Haptic Feedback**: $330\text{ Hz}$ ($0.98$) high-voltage arc detonation blast followed by $70\text{ Hz}$ ($0.90$) forearm bone recoil.

## Consequences
- Bridges overhead vault drop strikes with aggressive close-quarters blunt impacts.
- Works in tandem with javelin vault thrusts, radial disarms, and induction overcharge.
