# ADR 0097: Subterranean Cryo-Stasis Chamber Biometric Bypass Override

## Status
Accepted

## Context
Across Butcher Bay's double-max cryo facilities and Dark Athena medical wings, computerized stasis pods are protected by dual biometric interlocks to prevent unauthorized hibernation pod awakenings. Riddick can circumvent locked cryogenic canopies by executing a manual two-handed maintenance override: inserting a scavenged keycard or bypass wire into the upper terminal with the off-hand while pulling the heavy hydraulic emergency breaker downward with the dominant hand.

In flat desktop gaming, this operation was reduced to simple interaction prompts or automated puzzle sequences. In VR, requiring coordinated bimanual placement and simultaneous manipulation creates an authentic engineering sabotage experience.

## Decision
1. **Bimanual Pod Console Alignment**:
   - Off-hand positioned at upper card/wire bypass terminal ($Y_{rel} \in [0.08, 0.32]\text{ m}, Z_{rel} \in [-0.38, -0.14]\text{ m}$) with Left Grip held ($> 0.50$).
   - Dominant hand positioned at lower hydraulic breaker lever ($Y_{rel} \in [-0.28, 0.04]\text{ m}, Z_{rel} \in [-0.38, -0.14]\text{ m}$) with dominant grip held ($> 0.50$).
2. **Synchronized Breaker Pull**:
   - Dominant hand executes downward pull velocity ($V_y < -0.50\text{ m/s}$) while off-hand remains steady in the bypass terminal ($|\vec{V}_{left}| < 0.25\text{ m/s}$).
3. **Interlock Magnetic Snap & Solenoid Clatter**:
   - Off-hand receives $220\text{ Hz}$ ($0.50$) electronic card read chirp; dominant hand receives $120\text{ Hz}$ ($0.80$) heavy magnetic breaker disconnect thud.
4. **Engine Input Injection**:
   - Dispatches emergency bypass sequence (`E` + `Enter`) with a $2000\text{ ms}$ reset threshold.

## Consequences
- Requires genuine bimanual dexterity, perfectly leveraging 6DOF motion tracking.
- Prevents false activations by enforcing simultaneous upper terminal stabilization and lower lever yank.
