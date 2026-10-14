# ADR 0068: Inmate Toothbrush Shank Whittling & Rapid Friction Sharpening

## Status
Accepted

## Context
In Butcher Bay prison blocks (Single Max, Double Max) and the slave labor quarters of the Dark Athena, inmates construct lethal contraband shanks from scavenged industrial scrap, plastic toothbrushes, and melted acrylic glass. Crafting or maintaining these improvised blades requires friction grinding against rough concrete cell floors and rough granite stone walls.

In the original game, acquiring upgraded shanks was handled through scripted trader interactions or inventory item swaps. To deepen prison survival immersion in VR, players should physically grind and whittle their shivs against environmental masonry while crouched.

## Decision
We implement physical shank whittling and friction sharpening:
1. **Stance & Proximity Kinematic Thresholds:**
   - Player must be physically crouched or in deep prone stance ($Y_{drop} > 0.28\text{ m}$).
   - Dominant controller positioned low near the floor ($Y_{local} < -0.45\text{ m}$) and in front of the body ($Z_{local} \in [-0.45, -0.15]\text{ m}$).
   - Dominant grip clutched ($Grip > 0.50$) securing the shank handle.
2. **Abrasive Friction Motion Detection:**
   - Fast reciprocating horizontal lateral strokes ($|V_x| > 0.65\text{ m/s}$).
   - Temporal cooldown between stroke ticks: $180\text{ ms}$.
3. **Engine Action Dispatch & Micro-Haptic Rasp:**
   - Each reciprocating stroke generates abrasive micro-haptic scraping pulses on the dominant controller ($0.50$ amplitude at $200\text{ Hz}$).
   - Every 5 sustained strokes, triggers an inventory refresh confirmation (`E` / Slot '3' re-equip) and a bright metallic hone resonance chime ($260\text{ Hz}$, $0.70$).

## Consequences
- Deepens prison survival atmosphere during stealth downtime in cell blocks.
- Crouching posture requirement ensures shank whittling is never triggered accidentally during standing melee engagements.
