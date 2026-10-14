# ADR 0077: Guard Stun Baton Heavy Two-Handed Defibrillator Slam

## Status
Accepted

## Context
In *Assault on Dark Athena* and *Escape from Butcher Bay*, the riot baton serves as an electric melee shock weapon. While one-handed jabs deliver rapid localized stuns (ADR 0055), confronting armored cybernetic drones, alpha guard mechs, or mob swarms requires an overwhelming area-of-effect electro-kinetic finisher.

## Decision
We implement a two-handed heavy defibrillator ground slam:
1. **Two-Handed Baton Grip:**
   - Both controllers clutched firmly ($Grip_{left} > 0.50 \land Grip_{right} > 0.50$).
   - Controller proximity within two-handed baton handle span ($|Pos_{left} - Pos_{right}| < 0.25\text{ m}$).
2. **Downward Defibrillator Plunge Velocity:**
   - Both hands raised above chest height before plunging downward at extreme velocity ($V_{y, left} < -1.30\text{ m/s} \land V_{y, right} < -1.30\text{ m/s}$).
   - Both dominant and off-hand triggers depressed ($Trigger_{right} > 0.60 \land Trigger_{left} > 0.60$) to bridge capacitor circuits.
3. **Engine Action Dispatch & Electro-Kinetic Shockwave Feedback:**
   - Dispatches heavy downward strike action (`S` + `LMB`).
   - Generates dual synchronized maximum-amplitude electro-shock haptics ($0.98$ amplitude at $280\text{ Hz}$ for $220\text{ ms}$) simulating the explosive discharge of the baton's dual capacitor bank.

## Consequences
- Adds an exhilarating tactical crowd-control finisher to blunt melee weapons.
- Stringent trigger and velocity thresholds prevent accidental activation.
