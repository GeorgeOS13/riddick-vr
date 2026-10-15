# ADR 0172: Subterranean Hydroponics Bio-Waste Incinerator Flame Sensor Quartz Sight-Glass Scrape

## Context
In the Subterranean Hydroponics bio-waste incinerator rooms, combustion safeguards require continuous optical verification of the burner pilot flame. These optical sensors look into the combustion chambers through circular quartz sight-glass ports. Pyrolytic soot and sticky bio-tar coat these ports, blinding the sensors and triggering automated emergency fuel cutoffs. Riddick must physically scrape the quartz window clear to restore burner operation.

## Decision
Implement physical quartz sight-glass scraping via reciprocating 6DOF motion tracking:
1. **Sight-Glass Port Contact**:
   - Dominant hand/scraper placed against sight-glass window:
     - $Z_{controller} \in [-0.40, -0.16]\text{ m}$.
     - $Y_{controller} \in [-0.10, 0.25]\text{ m}$.
     - $Grip_{dominant} > 0.55$.
2. **Reciprocating Friction Sweeps**:
   - Executing planar friction strokes across the sight-glass plane:
     - $|\vec{V}_{xy}| = \sqrt{V_x^2 + V_y^2} > 0.52\text{ m/s}$.
   - Accumulating 4 distinct wiping cycles ($> 200\text{ ms}$ interval).
3. **Synthesized Game Controls**:
   - Upon the 4th cycle, synthesizes `E` (Interact) + `Tab` (Sensor reset) + `Enter` (Safety bypass) for $160\text{ ms}$.
4. **Haptic Feedback**:
   - $220\text{ Hz}$ ($0.45$) quartz rasp friction on each stroke.
   - Resonant $320\text{ Hz}$ ($0.85$) optical sync confirmation chime on completion.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Deepens industrial facility puzzle interactions and maintenance gameplay.
- Seamlessly integrates with the suite of incinerator systems.
