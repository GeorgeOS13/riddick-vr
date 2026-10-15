# ADR 0157: Inmate Pit Brawl Duck-Under Kidney Liver Counter-Jab

## Status
Accepted

## Context
When facing aggressive brawlers throwing wide overhead rights or haymakers in Butcher Bay's pit fights, upright blocking can still result in heavy chip damage or stamina drain. Riddick excels at dropping beneath the arc of a punch and delivering a compact, bone-crunching counter-punch directly into the opponent's exposed flank, targeting the kidney and lower floating ribs.

In VR, rewarding physical ducking with an immediate lateral flank counter-jab incentivizes athletic evasive head movement.

## Decision
We implement **Inmate Pit Brawl Duck-Under Kidney Liver Counter-Jab**:
1. **Low Evasive Posture**:
   - Player is physically crouched or in deep prone ($s\_isPhysicallyCrouched \lor s\_isDeepProne$).
2. **Flank Counter-Jab Impulse**:
   - Driving the dominant fist forward and slightly across ($|V_x| > 0.85\text{ m/s} \land V_z < -1.05\text{ m/s}$) into the lower torso corridor ($Y \in [-0.35, -0.05]\text{ m}$).
3. **Input Dispatch**:
   - Dispatches `C` (crouch lock) + `D` (lateral angle) + `LMB` (primary attack).
4. **Haptic Feedback**:
   - Dominant hand delivers a devastating $0.98$ visceral renal shockwave at $220\text{ Hz}$.

## Consequences
- Elevates physical crouching from mere stealth traversal into an active combat evasion technique.
- $1500\text{ ms}$ cooldown ensures clean strike registration.
