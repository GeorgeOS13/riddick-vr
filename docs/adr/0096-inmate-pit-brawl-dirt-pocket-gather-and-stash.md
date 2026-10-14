# ADR 0096: Inmate Pit Brawl Dirt / Ash Pocket Concealment

## Status
Accepted

## Context
In Butcher Bay's subterranean fighting arena and recreational yard brawls, blinding adversaries with floor dirt, gravel, or ash (ADR 0065 & 0085) is an essential dirty fighting tactic. Inmates frequently prepare for ambushes by scooping fine grit or abrasive soot from between floor paving stones while crouching and stashing the particulate into a waistband pocket or palm cup.

In standard flat games, ammunition or consumable gathering was abstracted away. In VR, preparing an improvised grit blind requires reaching down to the floor while physically crouched, performing a scooping motion, and touching the hand to the waist to conceal the grit until thrown.

## Decision
1. **Physical Crouch / Floor Proximity**:
   - Player must be physically crouched or in deep prone ($currentY < -0.28\text{ m}$).
   - Controller positioned near the ground surface ($Y_{rel} < -0.45\text{ m}$).
2. **Floor Scoop Motion**:
   - Hand open then closing into grip ($Grip > 0.50$) with a rearward dragging scoop ($V_z > 0.35\text{ m/s}$).
3. **Waist Stash Motion**:
   - Hand subsequently raised to waist/hip elevation ($Y_{rel} \in [-0.38, -0.15]\text{ m}, |X_{rel}| > 0.12\text{ m}$) within $1200\text{ ms}$.
4. **Tactile Sifting & Fabric Rustle Feedback**:
   - Floor contact generates $140\text{ Hz}$ ($0.35$) abrasive grit sifting micro-haptics; waist touch produces $180\text{ Hz}$ ($0.45$) cloth pocket rustle confirmation.
5. **Engine Input Injection**:
   - Primes pocket grit inventory (`E` + `Tab`) with a $2000\text{ ms}$ gathering deadband.

## Consequences
- Deepens the tactile preparation and roleplay of prison yard survival.
- Grounds the subsequent grit throwing gestures (ADR 0065 & 0085) with an authentic physical acquisition cycle.
