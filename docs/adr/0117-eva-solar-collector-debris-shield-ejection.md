# ADR 0117: Dark Athena EVA Solar Collector Debris Shield Ejection

## Status
Accepted

## Context
While navigating the exterior hull of the Dark Athena, micrometeoroid storms shatter photovoltaic debris shields, causing shrapnel to jam collector rotation. To restore mobility to the solar wings, Riddick can grab the dual jettison handles at the truss pivot and wrench rearward to blow the explosive retention pins and discard the damaged shielding.

## Decision
1. **Truss Shield Release Handle Stance**:
   - Both hands positioned at collector truss mount ($Z_{local} \in [-0.40, -0.14]\text{ m}, Y_{local} \in [0.10, 0.35]\text{ m}$).
   - Both grips clenched securely ($Grip_{left} > 0.55 \land Grip_{right} > 0.55$).
2. **Rearward Explosive Jettison Yank**:
   - Both hands hauled violently backward:
     - $V_{z, left} > 0.85\text{ m/s} \land V_{z, right} > 0.85\text{ m/s}$.
3. **Explosive Bolt Decoupling Haptics**:
   - Dual-controller pyrotechnic pin severance shockwave ($240\text{ Hz}$, $0.95$, $80\text{ ms}$) followed by cold-gas separation exhaust hiss ($130\text{ Hz}$, $0.50$, $120\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches emergency panel jettison signal (`E` + `Space` + `Enter`) with a $2500\text{ ms}$ lockout.

## Consequences
- Expands high-stakes exterior EVA spacewalk scenarios with cinematic zero-G environmental problem solving.
- Interlocks smoothly with solar array rotation (ADR 0114) and solar flare storm bulkheads (ADR 0109).
