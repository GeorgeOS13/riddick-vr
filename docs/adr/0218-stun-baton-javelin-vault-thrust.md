# ADR 0218: Security Guard Stun Baton Forward Two-Handed Javelin Vault Leap Thrust

## Status
Accepted

## Context
Crossing tactical corridors under fire or closing distance rapidly against fortified guards requires leaping forward while channeling kinetic inertia through a two-handed energized stun baton.

## Decision
Implement physical javelin vault leap thrust in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: In two-handed stance with stun baton energized ($dist < 0.22\text{ m}$, both grips $> 0.65$, Right Trigger $> 0.65$).
- **Dynamic Leap Thrust**: During forward vertical jump ($s\_isJumping \lor V_y > 0.85\text{ m/s}$), drive both arms explosively forward ($rVel.z < -1.40\text{ m/s} \land lVel.z < -1.20\text{ m/s}$) to deliver an airborne javelin strike.
- **Synthesized Controls**: Emits `Space` + `W` + `Q` + `LMB` for airborne leap, forward momentum, arc detonation, and penetrative shock.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) airborne arc detonation blast followed by dual $60\text{ Hz}$ ($0.90$) landing deck recoil.

## Consequences
- Bridges athletic movement with aggressive long-reach melee attacks.
- Complements overhead vault drop strikes, radial disarms, and javelin spear thrusts.
