# ADR 0171: Dark Athena EVA Solar Collector Heat Radiator Shutter Louver Multi-Slit Slide

## Context
During solar storms or high-power orbital maneuvers outside the *Dark Athena*, exterior radiator surfaces risk thermal degradation from intense radiative flux. The thermal radiators are fitted with banks of articulating multi-slit louver shutters that can be drawn shut across the cooling fins. Riddick can manually grasp the shutter actuator slider bar and slide it into the closed and locked position.

## Decision
Implement physical EVA radiator louver shutter sliding via 6DOF spatial tracking:
1. **Shutter Slider Bar Contact**:
   - Dominant hand positioned at shutter actuator track:
     - $Z_{controller} \in [-0.42, -0.15]\text{ m}$.
     - $Y_{controller} \in [-0.15, 0.25]\text{ m}$.
     - $Grip_{dominant} > 0.60$.
2. **Linear Louver Slide Motion**:
   - Sliding the bar vertically downward or horizontally across:
     - $|V_{y, dominant}| > 0.70\text{ m/s} \lor |V_{x, dominant}| > 0.70\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `E` (Interact) + `C` (Crouch / Close seal) + `Tab` (Subsystem toggle) for $160\text{ ms}$.
4. **Haptic Feedback**:
   - $160\text{ Hz}$ ($0.60$) metallic louver interlocking slide friction rasp.
   - $280\text{ Hz}$ ($0.90$) mechanical detent latch lock pulse when fully closed.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Enhances exterior shipboard survivability and puzzle mechanics during orbital spacewalk sections.
- Natural tactile feedback provides clear indication of shutter seal completion.
