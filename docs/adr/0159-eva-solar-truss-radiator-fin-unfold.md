# ADR 0159: Dark Athena EVA Solar Collector Truss Thermal Radiator Fin Unfolding Wrench

## Context
During EVA spacewalks outside the *Dark Athena*, stellar radiation exposure rapidly overheats exterior power conditioning buses and solar collector hubs unless excess thermal energy is continuously radiated into space. The ship's exterior trusses feature deployable thermal radiator fins folded flush against structural spars during combat or micro-meteoroid storms. Riddick must physically grasp the folded fin latch bracket and wrench the radiator fin outward to unfold its heat dissipation panels, restoring auxiliary power flow and cooling local relay conduits.

## Decision
Implement physical EVA solar collector truss thermal radiator fin unfolding via 6DOF spatial gestures:
1. **Folded Bracket Contact**:
   - Reaching dominant hand into radiator fin latch volume:
     - $Z_{controller} \in [-0.42, -0.15]\text{ m}$ (in front of chest/chassis plane).
     - $Y_{controller} \in [0.05, 0.38]\text{ m}$ (upper truss mounting plane).
     - $Grip_{dominant} > 0.60$ (firm clamp on radiator fin deployment lug).
2. **Outward Unfolding Wrench Sweep**:
   - Delivering an explosive lateral unfolding sweep outward ($|V_{x, dominant}| > 0.75\text{ m/s}$) to pivot the fin on its articulation hinge.
3. **Synthesized Game Controls**:
   - Emits `E` (Interact) + `W` (Forward throttle) + `Tab` (Systems toggle) for $140\text{ ms}$.
4. **Haptic Feedback**:
   - $170\text{ Hz}$ ($0.60$) geared hinge unseating friction during the sweep.
   - $280\text{ Hz}$ ($0.90$) detent latch lock pulse when fully deployed.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Enables physical exterior spacewalk heat dissipation and auxiliary systems maintenance.
- Integrates seamlessly into the Dark Athena EVA navigation suite without interfering with weapon handling or thruster navigation.
