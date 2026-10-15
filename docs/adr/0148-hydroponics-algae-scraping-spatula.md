# ADR 0148: Subterranean Hydroponics Algae Scraping Spatula Reciprocating Blade Stroke

## Status
Accepted

## Context
In the subterranean hydroponics bays of Butcher Bay and the Dark Athena, dense bio-film and luminescent algae encrust the quartz growth lamps, blocking photosynthetically active radiation and shutting down air revitalization loops. Riddick must physically clean the encrusted quartz tubes using a stamped steel scraper spatula to restore illumination and reveal hidden security passages.

In VR, direct physical blade contact with glass and bio-crust delivers rich tactile sensory immersion.

## Decision
We implement **Subterranean Hydroponics Algae Scraping Spatula Reciprocating Blade Stroke**:
1. **Scraping Plane Envelope**:
   - Controller positioned along the grow-tube surface ($Z \in [-0.40, -0.16]\text{ m}$, $Y \in [-0.25, 0.20]\text{ m}$).
   - Dominant grip held ($> 0.55$) maintaining blade alignment.
2. **Reciprocating Friction Strokes**:
   - Executing 4 rapid back-and-forth friction strokes ($|\vec{V}_{xy}| > 0.55\text{ m/s}$) sweeps encrusted algae away from the glass.
3. **Input Dispatch**:
   - Dispatches `E` (interact) + `Tab` (system inspection) + `Space` (environmental state cycle).
4. **Haptic Feedback**:
   - Gritty $210\text{ Hz}$ ($0.48$) abrasive rasp haptics on each stroke, culminating in a pristine $290\text{ Hz}$ ($0.75$) quartz resonance chime upon completing the 4th pass.

## Consequences
- Complements the existing botanical harvesting and sludge pump mechanics.
- Fully debounced with a $1500\text{ ms}$ reset period.
