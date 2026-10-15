# ADR 0133: Security Guard Stun Baton Rib-Jab & Ground Takedown

## Status
Accepted

## Context
When wielding a captured security riot stun baton, standard horizontal swings may be deflected by armored riot gear. Riddick exploits vulnerable gaps in tactical armor by plunging the live electrode tip into an adversary's lower floating ribs and forcefully driving the victim into the ground.

## Decision
1. **Energized Weapon Grip**:
   - Dominant hand holding stun baton with trigger squeezed ($Trigger_{right} > 0.60 \land Grip_{right} > 0.60$).
2. **Downward-Forward Stun Driving Stroke**:
   - High velocity combined forward and downward thrust:
     - $V_{z, right} < -1.10\text{ m/s} \land V_{y, right} < -0.85\text{ m/s}$.
   - Strike impact corridor: $Y_{local, right} \in [-0.30, 0.05]\text{ m}, Z_{local, right} \in [-0.35, -0.10]\text{ m}$.
3. **Electro-Shock Rib Concussion & Ground Impact Haptics**:
   - Initial flesh contact: Severe $290\text{ Hz}$ ($0.98$, $80\text{ ms}$) neuromuscular shockwave.
   - Followed by deck impact reverberation ($60\text{ Hz}$, $0.90$, $140\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches brutal baton ground pin combination (`S` + `Space` + `LMB`) with a $1500\text{ ms}$ lockout.

## Consequences
- Enriches tactical melee combat when wielding electrified weapons, offering an aggressive alternative to defensive arc shockwaves (ADR 0118).
- High dual-axis velocity threshold guarantees intentional execution during heated close-quarters combat.
