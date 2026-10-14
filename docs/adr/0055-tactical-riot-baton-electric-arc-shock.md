# ADR 0055: Tactical Riot Baton Electric Arc Shock & Jab

## Status
Accepted / Implemented

## Context
Butcher Bay corrections officers and Dark Athena security units carry electrified stun batons designed to incapacitate unruly prisoners. When Riddick disarms a guard or acquires a riot baton, flat-screen controls map attacks to generic light/heavy swings. In virtual reality, the electrified riot baton provides tactile distinctions between blunt kinetic clubbing and high-voltage arc discharges.

## Decision
We implement tactical baton kinetic jabbing and high-voltage electrostatic arcing in Section 5a/8 of `src/hook/camera_hook.cpp`:
1. **Kinetic Stun Jab**:
   - Forward thrust with dominant hand ($V_{z, right} < -1.10\text{ m/s}$) dispatches Primary Attack (`LMB` down/up), delivering blunt impact with $0.85$ ($140\text{ Hz}$) kinetic recoil.
2. **High-Voltage Electrostatic Arc Discharge**:
   - Squeezing the dominant trigger ($> 0.60$) during a thrust ($V_{z, right} < -0.90\text{ m/s}$) channels high-voltage capacitor current:
     Dispatches Primary Attack (`LMB`) coupled with electrical pulse dispatch.
   - Emits an intense $260\text{ Hz}$ ($0.90$ amplitude) electrical sizzle shockwave across the dominant hand.

## Consequences
- Distinguishes physical kinetic swings from electrostatic arc discharges via trigger engagement.
- High frequency ($260\text{ Hz}$) haptics convey electrical current vibration.
- Prevents accidental arc discharges by requiring deliberate trigger pull during forward thrusts.
