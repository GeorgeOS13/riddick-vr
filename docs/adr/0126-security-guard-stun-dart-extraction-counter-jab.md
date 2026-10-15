# ADR 0126: Security Guard Stun Dart Extraction & Counter-Jab

## Status
Accepted

## Context
When ambushed by guard squads wielding tranq rifles or taser dart projectors, Riddick can be hooked by barbed electrode darts delivering neuromuscular incapacitation. Rather than succumbing to the voltage, Riddick seizes the embedded dart from his chest and drives it directly into the neck or ocular socket of an attacking guard, reversing the shock arc.

## Decision
1. **Chest Dart Extraction Grab & Pull**:
   - Dominant hand positioned at chest puncture zone ($|X| < 0.16\text{ m}, Y \in [-0.25, 0.05]\text{ m}, Z \in [-0.25, -0.06]\text{ m}$).
   - Grip clenched ($Grip_{right} > 0.65$) and pulled forward/outward violently ($V_{z, right} > 0.70\text{ m/s} \lor |V_{x, right}| > 0.75\text{ m/s}$).
   - Primes the extracted dart weapon in hand for a $750\text{ ms}$ reaction window.
2. **Aggressive Counter-Jab Strike**:
   - Within the reaction window, dominant hand drives forward into head/neck level ($V_{z, right} < -1.15\text{ m/s}, Y_{local, right} \in [-0.10, 0.22]\text{ m}$).
3. **Barbed Flesh Extraction & Electro-Shock Retaliation Haptics**:
   - Extraction: Sharp tearing snap ($280\text{ Hz}$, $0.75$, $45\text{ ms}$).
   - Counter-jab: Catastrophic $320\text{ Hz}$ ($0.98$, $160\text{ ms}$) electrostatic pulse discharging through the target.
4. **Engine Input Injection**:
   - Dispatches reversal takedown sequence (`E` + `3` + `LMB`) with a $1500\text{ ms}$ lockout.

## Consequences
- Provides a brutal counter-mechanic against long-range tranquilizer and stun dart ambushes.
- Complements taser wire snipping (ADR 0101) with an aggressive physical retaliation attack.
