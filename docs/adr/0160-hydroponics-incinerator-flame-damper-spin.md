# ADR 0160: Subterranean Hydroponics Bio-Waste Incinerator Flame Damper Handwheel Spin

## Context
In the Subterranean Hydroponics facility, biological contaminants and toxic fungal blooms must be purged through heavy industrial incinerators. Operating these combustion chambers requires balancing intake air oxygenation and burner exhaust through heavy cast-iron flame damper vanes. The flue dampers are manually actuated via a geared spoked handwheel mounted directly above the combustion manifold. Riddick must physically grasp the handwheel and spin it through multiple revolutions to open the damper and provide sufficient draft for high-temperature incineration.

## Decision
Implement physical bio-waste incinerator flame damper handwheel rotation via continuous 6DOF motion tracking:
1. **Handwheel Contact**:
   - Dominant hand positioned at flue damper mounting plane:
     - $Z_{controller} \in [-0.42, -0.16]\text{ m}$.
     - $Y_{controller} \in [-0.25, 0.15]\text{ m}$.
     - $Grip_{dominant} > 0.55$.
2. **Rotary Cranking Motion**:
   - Tracking circular displacement velocity in the frontal $XY$ plane:
     - $|\vec{V}_{xy}| = \sqrt{V_x^2 + V_y^2} > 0.48\text{ m/s}$.
   - Accumulating angular displacement across 3 full revolutions (or 3 distinct tangential velocity arcs).
3. **Synthesized Game Controls**:
   - Upon completing the 3rd rotation, synthesizes `E` (Interact) + `Space` (Auxiliary actuator) + `Q` (Pneumatic valve toggle) for $160\text{ ms}$.
4. **Haptic Feedback**:
   - $130\text{ Hz}$ ($0.55$) threaded spindle rotation rumble on each revolution.
   - $210\text{ Hz}$ ($0.95$) mechanical end-stop detent thud when the damper vanes lock fully open.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Deepens industrial facility environmental interactions and environmental sabotage puzzles.
- Complements the spark wheel igniter (ADR 0156) and dual blast door levers (ADR 0152) for full incinerator system operation.
