# ADR 0129: Dark Athena EVA Solar Panel Photovoltaic Cell Scrape & De-icing

## Status
Accepted

## Context
During exterior hull traverses on the Dark Athena, micrometeorite frost and carbon soot deposit onto the exterior solar arrays, draining emergency auxiliary reserves and locking hull access doors. Riddick can scrape frozen deposits from the photovoltaic surface to restore solar power bus feeds.

## Decision
1. **Solar Array Pane Proximity**:
   - Controller positioned in surface contact zone ($Z \in [-0.42, -0.14]\text{ m}, Y \in [-0.20, 0.25]\text{ m}$).
   - Grip held firmly ($Grip > 0.55$).
2. **Reciprocating Surface Scraping Friction**:
   - Requires 4 reciprocating lateral or vertical scrape strokes ($|V_x| > 0.60\text{ m/s} \lor |V_y| > 0.60\text{ m/s}$).
   - Strokes must occur within $2500\text{ ms}$ of each other.
3. **Crystalline Glass Scraping Rasp & Power Bus Sync Haptics**:
   - Each stroke generates an abrasive rasp pulse ($240\text{ Hz}$, $0.50$, $40\text{ ms}$).
   - On the fourth stroke, delivers an auxiliary power restoration electrical resonance pulse ($310\text{ Hz}$, $0.80$, $150\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches array power recovery command (`E` + `Tab` + `Enter`) with a $1500\text{ ms}$ lockout.

## Consequences
- Deepens exterior spacewalk maintenance activities alongside solar array hand cranking (ADR 0114) and antenna polarization alignment (ADR 0125).
- Provides tactile puzzle progression without interrupting first-person VR immersion.
