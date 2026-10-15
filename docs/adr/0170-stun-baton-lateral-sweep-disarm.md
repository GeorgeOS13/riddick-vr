# ADR 0170: Security Guard Stun Baton Lateral Cross-Sweep Deflection & Disarm

## Context
When facing aggressive enemies wielding shivs, bludgeons, or bayonets, the player can use the energized stun baton to execute a horizontal cross-sweep parry. The electrified baton surface makes contact with the incoming weapon, conducting high-voltage capacitive current directly into the attacker's hands and arms, inducing muscular spasm and forcing them to drop their weapon immediately.

## Decision
Implement lateral cross-sweep deflection and disarm via 6DOF combat gesture tracking:
1. **Energized Baton Grip**:
   - Baton energized and ready:
     - $Grip_{dominant} > 0.60$.
     - $Trigger_{dominant} > 0.65$.
     - $Y_{dominant} \in [-0.20, 0.20]\text{ m}$ (held at chest/torso level).
2. **Horizontal Lateral Cross-Sweep**:
   - Sweeping the baton violently across the frontal plane:
     - $|V_{x, dominant}| > 1.25\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `A` (if sweeping left) or `D` (if sweeping right) + `RMB` (Block/Parry) + `E` (Disarm interact) for $180\text{ ms}$.
4. **Haptic Feedback**:
   - $310\text{ Hz}$ ($0.98$) electromagnetic repulsion arc blast on dominant hand.
   - Secondary $0.65$ wrist recoil pulse.
5. **Cooldown**: $850\text{ ms}$ debounce.

## Consequences
- Adds an active reactive parry and disarm mechanism for stun baton combat.
- Rewards fast lateral defensive reactions against aggressive melee foes.
