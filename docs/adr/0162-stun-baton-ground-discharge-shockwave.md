# ADR 0162: Security Guard Stun Baton Dual-Hand Ground Discharge Kinetic Shockwave

## Context
The security guard stun baton possesses internal high-output magnetic induction capacitors capable of delivering lethal electrical arcs. When wielded with both hands and grounded directly into metal floor plating or deck grating, the entire charge can be discharged through structural deck seams. This creates a radiating electrostatic ground wave that stuns and knocks back surrounding adversaries in a 360-degree perimeter around the player.

## Decision
Implement two-handed stun baton ground discharge shockwave via synchronized 6DOF plunge detection:
1. **Two-Handed Energized Stance**:
   - Hands in close proximity along the baton shaft ($dist < 0.22\text{ m}$).
   - Both grips clenched: $Grip_{right} > 0.65$, $Grip_{left} > 0.60$.
   - Both triggers held: $Trigger_{right} > 0.65$, $Trigger_{left} > 0.60$.
2. **Synchronized Downward Deck Plunge**:
   - Both hands drive aggressively downward toward the deck:
     - $V_{y, right} < -1.25\text{ m/s}$.
     - $V_{y, left} < -1.25\text{ m/s}$.
     - Floor contact height reached: $Y < -0.40\text{ m}$.
3. **Synthesized Game Controls**:
   - Synthesizes `S` (Rearward brace) + `Space` (Ground shock) + `Q` (Arc discharge) + `LMB` (Primary attack) for $220\text{ ms}$.
4. **Haptic Feedback**:
   - Dual-hand $50\text{ Hz}$ ($0.98$) deep seismic floor impact shockwave.
   - Followed by $320\text{ Hz}$ ($0.98$) high-frequency electrostatic dielectric breakdown crackle.
5. **Cooldown**: $1000\text{ ms}$ debounce.

## Consequences
- Gives the player a powerful high-risk, high-reward crowd control defensive option when surrounded by guards or inmates.
- Seamlessly integrates with the stun baton weapon family without interfering with standard strikes or parries.
