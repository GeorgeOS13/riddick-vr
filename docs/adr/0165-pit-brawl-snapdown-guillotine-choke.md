# ADR 0165: Inmate Pit Brawl Collar Tie Snap-Down & Guillotine Choke

## Context
In the inmate brawl pits, opponents often charge aggressively with their heads lowered. Rather than stepping back, Riddick can clamp onto the back of the opponent's neck with a collar tie, violently snap their head downward to break their posture, and immediately wrap his dominant arm under the chin to lock in a lethal standing guillotine choke.

## Decision
Implement physical collar tie snap-down followed by guillotine choke lock via 6DOF combat gesture tracking:
1. **Off-Hand Collar Tie Snap-Down**:
   - Off-hand grasps opponent's neck corridor and pulls downward:
     - $Z_{left} \in [-0.35, -0.12]\text{ m}$.
     - $Grip_{left} > 0.65$.
     - $V_{y, left} < -0.90\text{ m/s}$ (violent downward snap).
2. **Dominant Guillotine Forearm Wrap**:
   - Within $500\text{ ms}$ of the snap-down:
     - Dominant forearm drives upward and forward under the chin:
       - $V_{y, right} > 1.05\text{ m/s}$.
       - $V_{z, right} < -0.75\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `S` (Rearward leverage) + `C` (Crouch choke lock) + `LMB` (Primary attack) for $180\text{ ms}$.
4. **Haptic Feedback**:
   - Off-hand receives $210\text{ Hz}$ ($0.90$) cervical snap-down jerk.
   - Dominant arm receives $280\text{ Hz}$ ($0.98$) tracheal compression chokehold pulse.
5. **Cooldown**: $850\text{ ms}$ debounce.

## Consequences
- Adds an authentic, devastating submission maneuver to Riddick's unarmed combat repertoire.
- Rewards aggressive infighting counters against rushing adversaries.
