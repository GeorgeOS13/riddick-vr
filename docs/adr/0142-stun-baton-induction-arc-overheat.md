# ADR 0142: Security Guard Stun Baton Weapon Shock Induction Arc Overheat

## Status
Accepted

## Context
Captured security riot batons are armed with high-voltage step-up transformers designed for single-target incapacitation. When confronting heavily armored riot enforcers or squads of cybernetic drones, Riddick can physically ground the baton's secondary electrode directly against his armored gauntlet or grounding sleeve while depressing the primary capacitor switches. This intentionally shorts the transformer coils, generating a catastrophic electromagnetic flash and radial induction arc overheat that shocks all nearby enemies and temporarily blinds security optics.

In VR, this creates a high-risk, high-reward two-handed charge mechanic requiring steady contact before triggering the discharge.

## Decision
We implement **Security Guard Stun Baton Weapon Shock Induction Arc Overheat**:
1. **Two-Handed Induction Grounding Stance**:
   - Hands held close together ($dist(Left, Right) < 0.16\text{ m}$).
   - Both controllers gripped firmly ($leftGrip > 0.60 \land rightGrip > 0.60$) with both triggers held ($leftTrigger > 0.65 \land rightTrigger > 0.65$).
2. **Thermal Overheat Saturation**:
   - Maintaining the grounded contact pose for $> 350\text{ ms}$ drives the induction coil to saturation.
   - Dynamic haptic feedback ramps smoothly from $0.35$ up to $0.90$ ($90\text{ Hz} \to 280\text{ Hz}$) as capacitor heat climbs.
3. **Overheat EMP Detonation & Input Dispatch**:
   - Upon reaching full saturation ($350\text{ ms}$), the coil detonates in an explosive blinding arc.
   - Dispatches `S` (recoil plant) + `Q` (electrostatic surge) + `LMB` (primary discharge).
4. **Haptic Feedback**:
   - Dual-hand $0.98$ EMP arc blast shockwave at $300\text{ Hz}$.

## Consequences
- Complements the existing radial arc wave (ADR 0118) and rib-jab takedown (ADR 0133) by offering a grounded overload detonation.
- $2500\text{ ms}$ cooldown ensures balanced pacing during high-intensity encounters.
