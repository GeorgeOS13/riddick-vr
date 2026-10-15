# ADR 0176: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Steam Scrubber Siphon Drain

## Context
Exhaust from the bio-waste incinerators in Subterranean Hydroponics passes through high-pressure aqueous scrubber columns to condense toxic acid gases and capture soot particulates. Sump water at the column base accumulates heavy sludge and must be periodically dumped into secondary processing sumps by manually yanking an emergency siphon drainage lever.

## Decision
Implement physical scrubber siphon drainage lever manipulation via 6DOF tracking:
1. **Sump Lever Grasp**:
   - Dominant hand positioned at low drainage pedestal:
     - $Z_{controller} \in [-0.42, -0.16]\text{ m}$.
     - $Y_{controller} \in [-0.50, -0.15]\text{ m}$.
     - $Grip_{dominant} > 0.55$.
2. **Upward Siphon Stroke**:
   - Hauling the lever upward firmly:
     - $V_{y, dominant} > 0.75\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `E` (Interact) + `Q` (Drain valve) + `Space` (Siphon purge) for $160\text{ ms}$.
4. **Haptic Feedback**:
   - $110\text{ Hz}$ ($0.65$) heavy mechanical spring tension pull.
   - Resonant $90\text{ Hz}$ ($0.90$) turbulent hydraulic drain surge rumble.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Complements the interconnected incinerator and hydroponic fluid transport networks.
- Enriches low-stance environmental manipulation.
