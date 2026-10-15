# ADR 0169: Inmate Pit Brawl Shin Trap & Overhand Hammerfist Ground Finisher

## Context
In bare-knuckle pit brawling, catching an opponent's low kick or checking their aggressive stride creates an opening for a catastrophic counter-attack. Riddick can use his off-hand to deflect and trap the incoming shin, stepping inward to deliver a vertical overhand hammerfist directly onto the opponent's collarbone or temple, driving them into the floor.

## Decision
Implement physical shin trap followed by overhand hammerfist finisher via 6DOF combat gesture tracking:
1. **Off-Hand Shin Trap / Scoop**:
   - Deflecting downward into low leg corridor:
     - $V_{y, left} < -0.85\text{ m/s}$.
     - $Y_{left} < -0.20\text{ m}$.
     - $Grip_{left} > 0.55$.
2. **Dominant Overhand Hammerfist Strike**:
   - Starting from chest or shoulder height and slamming downward:
     - $V_{y, right} < -1.30\text{ m/s}$.
     - $Y_{right}$ transits downward ($Y_{start} > -0.10\text{ m} \to Y_{end} < -0.30\text{ m}$).
3. **Synthesized Game Controls**:
   - Synthesizes `S` (Rearward stance) + `Space` (Takedown leverage) + `LMB` (Hammerfist strike) for $180\text{ ms}$.
4. **Haptic Feedback**:
   - Off-hand receives $180\text{ Hz}$ ($0.75$) shin trap deflection impact.
   - Dominant hand receives $280\text{ Hz}$ ($0.98$) skeletal bone-cracking hammerfist concussive shockwave.
5. **Cooldown**: $850\text{ ms}$ debounce.

## Consequences
- Broadens high-low infighting techniques in unarmed pit brawls.
- Gives the player an intuitive, brutal downward kinetic strike.
