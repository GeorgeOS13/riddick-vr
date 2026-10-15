# ADR 0128: Inmate Pit Brawl Double-Ear Concussive Clap Strike

## Status
Accepted

## Context
In Butcher Bay's pit brawls and cafeteria ambushes, heavily built inmates absorb conventional jabs and body blows. To disrupt equilibrium, rupture eardrums, and induce instant sensory disorientation, Riddick executes a concussive open-palm double ear clap directly against the sides of the skull.

## Decision
1. **Transition from Boxer Guard to Open-Palm Strike**:
   - Player initiates from active Boxer Guard or chin-level fighting stance.
   - Both hands opened ($Trigger_{left} < 0.25 \land Trigger_{right} < 0.25 \land Grip_{left} < 0.35 \land Grip_{right} < 0.35$).
2. **Inward Converging Horizontal Palm Acceleration**:
   - Both hands accelerate inward toward the midline ($V_{x, left} > 0.95\text{ m/s} \land V_{x, right} < -0.95\text{ m/s}$).
   - Strike corridor: $Y \in [-0.10, 0.20]\text{ m}, Z \in [-0.35, -0.10]\text{ m}$, final inter-hand spacing $dist < 0.24\text{ m}$.
3. **Dual Concussive Shockwave & Acoustic Ringing Haptics**:
   - Dual-hand immediate concussive blast ($160\text{ Hz}$, $0.98$, $80\text{ ms}$).
   - Followed by sustained vestibular disorientation rumble ($45\text{ Hz}$, $0.50$, $250\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches heavy stagger takedown command (`W` + `Space` + `LMB`) with a $1500\text{ ms}$ lockout.

## Consequences
- Broadens unarmed pit fighting with an explosive disorienting technique that counters aggressive charging opponents.
- Requires high inward velocities to prevent false positives when naturally clapping or adjusting controller grip.
