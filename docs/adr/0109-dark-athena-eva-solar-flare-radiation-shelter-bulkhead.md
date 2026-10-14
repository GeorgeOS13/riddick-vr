# ADR 0109: Dark Athena EVA Solar Flare Radiation Shelter Bulkhead

## Status
Accepted

## Context
During exterior hull spacewalks on the Dark Athena, intense solar radiation sweeps across the exposed hull plating. Riddick must seek shelter inside recessed emergency niches and pull down heavy lead-reinforced storm bulkheads before lethal radiation flushes the exterior deck.

## Decision
1. **Overhead Storm Bulkhead Latch Zone**:
   - Both hands elevated above eye level ($Y_{local} > 0.22\text{ m}, Z_{local} \in [-0.30, 0.05]\text{ m}$).
   - Both grips clenched securely ($Grip_{left} > 0.60 \land Grip_{right} > 0.60$).
2. **Violent Downward Haul Gesture**:
   - Both hands hauled downward violently ($V_{y, left} < -1.10\text{ m/s} \land V_{y, right} < -1.10\text{ m/s}$).
3. **Lead Bulkhead Slam Haptics**:
   - Immense $40\text{ Hz}$ ($0.98$) deep structural thud as the massive lead door slams into deck retainers.
   - Secondary high-frequency pneumatic seal hissing ($200\text{ Hz}$, $0.40$, $120\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches emergency radiation bulkhead dogging sequence (`E` + `C` + `Space`) with a $2500\text{ ms}$ lockout.

## Consequences
- Elevates tension during high-intensity EVA solar storm survival sequences.
- Prevents false activations during normal magnetic boot locomotion by requiring dual-hand overhead elevation and simultaneous high downward velocity.
