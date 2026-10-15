# ADR 0174: Security Guard Stun Baton Dual-Hand Cross-Block Reactive Arc Burst

## Context
When facing overwhelmed odds against multiple charging adversaries, a guard or inmate equipped with an electrified stun baton can form a reinforced cross-wrist X-block. Squeezing both triggers pre-charges the baton's capacitor array, and thrusting outward at the moment of impact releases an explosive reactive arc burst that repels and stuns all frontal attackers.

## Decision
Implement dual-hand cross-block reactive arc burst via 6DOF combat tracking:
1. **Cross-Wrist Pre-Charge Stance**:
   - Both hands crossed in close proximity in front of chest:
     - $dist(L, R) < 0.16\text{ m}$.
     - $Y_{controllers} \in [-0.15, 0.15]\text{ m}$.
     - $Z_{controllers} \in [-0.30, -0.10]\text{ m}$.
     - Both grips clenched: $Grip_{left} > 0.65 \land Grip_{right} > 0.65$.
     - Both triggers depressed: $Trigger_{left} > 0.65 \land Trigger_{right} > 0.65$.
   - Dwell duration: $> 300\text{ ms}$.
2. **Explosive Forward Reactive Thrust**:
   - Driving both hands forward aggressively:
     - $V_{z, right} < -1.15\text{ m/s} \lor V_{z, left} < -1.15\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `RMB` (Block) + `Space` (Reactive thrust) + `Q` (Arc discharge) for $180\text{ ms}$.
4. **Haptic Feedback**:
   - Dual-hand $240\text{ Hz}$ ($0.60$) magnetic capacitor charging hum during dwell.
   - Concussive dual $320\text{ Hz}$ ($0.98$) electrostatic repulsion arc detonation shockwave.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Provides a defensive counter-burst option that rewards precise timing.
- Perfectly expands the stun baton's tactical high-voltage moveset.
