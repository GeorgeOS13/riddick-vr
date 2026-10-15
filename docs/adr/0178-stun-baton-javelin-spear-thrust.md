# ADR 0178: Security Guard Stun Baton Forward Two-Handed Javelin Thrust

## Context
When engaging heavily armed or armored hostiles at the outer perimeter of melee reach, security guards use a two-handed linear javelin grip on the stun baton. The dominant hand grips the pommel to supply driving force, while the off-hand loosely cradles the forward shaft to guide its trajectory. Thrusting forward with both hands delivers the energized electrode directly into the target's sternum with maximum penetration and leverage.

## Decision
Implement two-handed stun baton forward javelin spear thrust via aligned 6DOF tracking:
1. **Two-Handed Aligned Shaft Stance**:
   - Hands spaced along weapon line:
     - $dist(L, R) \in [0.18, 0.40]\text{ m}$.
     - $Grip_{right} > 0.60 \land Grip_{left} > 0.50$.
     - $Trigger_{right} > 0.65$ (capacitors energized).
2. **Explosive Linear Penetration Thrust**:
   - Both hands driving straight forward along the view axis:
     - $V_{z, right} < -1.35\text{ m/s} \land V_{z, left} < -1.05\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `W` (Forward lunge) + `Space` (Linear penetration) + `LMB` (Primary attack) + `Q` (Arc discharge) for $200\text{ ms}$.
4. **Haptic Feedback**:
   - Dominant hand receives $320\text{ Hz}$ ($0.98$) direct linear electro-kinetic impact pulse.
   - Off-hand receives $140\text{ Hz}$ ($0.65$) forward shaft guide friction vibration.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Expands long-range piercing options with the stun baton.
- Rewards natural two-handed spearing posture and momentum.
