# ADR 0166: Security Guard Stun Baton Charged High-Arc Downward Skull Breaker

## Context
When facing heavily armored riot guards or mutated inmates resistant to glancing blows, the security guard stun baton can be charged to full capacitor voltage and swung downward from a high overhead arch. The concussive electro-kinetic kinetic energy delivered directly to the skull shatters ballistic helmets and incapacitates targets instantly.

## Decision
Implement charged high-arc downward skull breaker cleave via 6DOF tracking:
1. **High Overhead Hoist Stance**:
   - Baton hoisted high above eye level:
     - $Y_{right} > 0.25\text{ m}$.
     - $Grip_{right} > 0.60$.
     - $Trigger_{right} > 0.65$ (capacitors energized).
2. **Extreme Downward Vertical Cleave**:
   - Driving the energized baton downward with extreme velocity:
     - $V_{y, right} < -1.45\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `W` (Forward step) + `S` (Heavy overhead stance) + `LMB` (Primary strike) for $180\text{ ms}$.
4. **Haptic Feedback**:
   - $340\text{ Hz}$ ($0.98$) high-energy electrostatic cranial detonation blast on impact.
   - Secondary $80\text{ Hz}$ ($0.85$) wrist recoil reverberation.
5. **Cooldown**: $850\text{ ms}$ debounce.

## Consequences
- Provides a devastating vertical melee finisher for stun baton combat.
- Rewards full physical motion and decisive overhead strikes.
