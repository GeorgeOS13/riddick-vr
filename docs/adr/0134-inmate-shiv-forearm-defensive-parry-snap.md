# ADR 0134: Inmate Shiv Forearm Defensive Parry-and-Snap

## Status
Accepted

## Context
Butcher Bay inmates frequently attempt heavy overhead club or shiv swings. Rather than merely absorbing the impact through Boxer Guard, Riddick can perform an active intercept: hooking the attacker's forearm outward with his off-hand while violently smashing the spine of his shiv or brass-knuckled fist downward onto the opponent's exposed elbow joint.

## Decision
1. **Initiation from Boxer Guard**:
   - Player maintains active Boxer Guard ($s\_currentBlockEngaged$).
2. **Coordinated Parry & Joint Snap Strike**:
   - Off-hand deflects incoming strike laterally:
     - $|V_{x, left}| > 0.85\text{ m/s}$.
   - Dominant hand drives downward sharply onto the trapped arm:
     - $V_{y, right} < -1.15\text{ m/s}$.
   - Controllers remain in close defensive proximity ($dist(Left, Right) < 0.28\text{ m}$).
3. **Bone Deflection & Joint Fracture Haptics**:
   - Off-hand: Forearm deflection snap ($220\text{ Hz}$, $0.80$, $40\text{ ms}$).
   - Dominant hand: Concussive hyperextension bone crack ($280\text{ Hz}$, $0.98$, $75\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches counter-parry disarm sequence (`W` + `E` + `LMB`) with a $1500\text{ ms}$ lockout.

## Consequences
- Elevates reactive melee depth by turning defensive blocks into proactive limb-breaking counter-attacks.
- Distinct from the straight forward counter-disarm (ADR 0018) and eye-gouge (ADR 0092) by targeting the opponent's weapon arm directly.
