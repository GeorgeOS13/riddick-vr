# ADR 0234: Security Guard Stun Baton Forward High-Arc Kinetic Cleave & Shockwave Stun

## Status
Accepted

## Context
Breaking past high heavy shields or groups of staggered guards calls for an overwhelming overhead arc strike. Raising the electrified stun baton overhead in a two-handed grip and bringing it down with maximum kinetic velocity cleaves through defenses and transmits a high-voltage shockwave across the metal deck.

## Decision
Implement physical stun baton kinetic cleave mechanics in `src/hook/camera_hook.cpp`:
- **High Overhead Two-Handed Stance**: Both hands raised above shoulder height (`handsDistSledge < 0.28\text{ m}`, `rightLocal.y > 0.18\text{ m}`, `leftLocal.y > 0.10\text{ m}`, left grip $> 0.65$, right grip $> 0.65$, right trigger $> 0.65$).
- **Downward Cleaving Velocity**: Snap both controllers downward with explosive velocity (`rVel.y < -1.40\text{ m/s} \land lVel.y < -1.10\text{ m/s}`).
- **Synthesized Controls**: Emits `S` + `W` + `Q` + `LMB` for stance plant, forward kinetic drive, arc discharge, and cleaving impact.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.98$) high-arc kinetic impact and plasma discharge on the striking hand, $240\text{ Hz}$ ($0.88$) dielectric dissipation on the secondary grip hand.

## Consequences
- Devastating offensive weapon strike capable of breaking guard and stunning nearby opponents.
- Seamlessly integrates with grounding thrusts, shield bashes, and hammerfist deck strikes.
