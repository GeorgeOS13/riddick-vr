# ADR 0136: Inmate Pit Brawl Dirt / Ash Eye-Rub Recovery Riposte

## Status
Accepted

## Context
During pit brawls in Butcher Bay, unscrupulous inmates throw dirt or hot ash into Riddick's face to blind him. Rather than retreating, Riddick wipes his eyes with his off-arm while immediately lunging forward with a brutal blind counter-punch or shiv riposte to punish the attacker.

## Decision
1. **Off-Hand Forearm Eye Wipe**:
   - Off-hand positioned across the eyes/forehead ($|X_{left}| < 0.14\text{ m}, Y_{left} \in [-0.08, 0.15]\text{ m}, Z_{left} \in [-0.22, -0.06]\text{ m}$).
   - Lateral wiping motion ($|V_{x, left}| > 0.50\text{ m/s}$).
   - Primes the recovery riposte window for $600\text{ ms}$.
2. **Immediate Dominant Riposte Strike**:
   - Within the $600\text{ ms}$ window, dominant hand drives forward aggressively:
     - $V_{z, right} < -1.15\text{ m/s}$.
3. **Corneal Clearance & Retaliatory Concussion Haptics**:
   - Off-hand: Rapid corneal wipe friction rasp ($190\text{ Hz}$, $0.40$, $40\text{ ms}$).
   - Dominant hand: Devastating blunt/piercing counter-strike blast ($240\text{ Hz}$, $0.98$, $110\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches blind recovery counter combination (`Space` + `W` + `LMB`) with a $1500\text{ ms}$ lockout.

## Consequences
- Seamlessly bridges defensive recovery mechanics (ADR 0073) with immediate lethal offense.
- Rewards aggressive instincts under sensory impairment.
