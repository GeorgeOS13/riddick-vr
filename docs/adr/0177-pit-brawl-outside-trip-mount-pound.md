# ADR 0177: Inmate Pit Brawl Outside Trip & Mount Ground-and-Pound

## Context
In Butcher Bay and Dark Athena inmate fighting pits, ending a fight quickly before guards intervene requires taking opponents down and maintaining dominant top control. When an adversary squares up or throws a telegraphed punch, Riddick can execute an outside leg reap/trip with his off-hand and lead leg while driving forward with his dominant fist, taking the opponent down and mounting their chest to deliver bone-shattering ground strikes.

## Decision
Implement outside trip takedown and ground-and-pound transition via 6DOF combat gesture tracking:
1. **Low Outside Trip Sweep**:
   - Off-hand sweeps low across the opponent's lead ankle:
     - $V_{x, left} < -0.80\text{ m/s}$.
     - $Y_{left} < -0.30\text{ m}$.
     - $Grip_{left} > 0.50$.
2. **Forward-Downward Dominant Drive**:
   - Dominant arm drives diagonally forward and down:
     - $V_{z, right} < -1.10\text{ m/s} \land V_{y, right} < -0.75\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `W` (Forward drive) + `C` (Mount ground transition) + `LMB` (Ground strike) for $200\text{ ms}$.
4. **Haptic Feedback**:
   - Off-hand receives $170\text{ Hz}$ ($0.80$) ankle hook resistance impact.
   - Dominant hand receives $260\text{ Hz}$ ($0.98$) concussive cranial deck pound shockwave.
5. **Cooldown**: $850\text{ ms}$ debounce.

## Consequences
- Broadens grappling takedowns into a full mount ground-and-pound sequence.
- Gives the player devastating close-range takedown authority.
