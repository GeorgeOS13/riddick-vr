# ADR 0090: Inmate Bunk Overhead Pipe Chin-Up & Ledge Pull Stance

## Status
Accepted

## Context
In Butcher Bay cell blocks and claustrophobic maintenance conduits, overhead steam pipes, conduit trays, and ventilation beams run across the ceiling. Riddick's iconic stealth repertoire includes grabbing overhead beams, performing a silent chin-up tuck, and suspending himself horizontally against the ceiling while armed guards pass directly underneath unaware.

In flat desktop gaming, ceiling hangs were automated contextual jump triggers. In VR, hoisting Riddick's body above guard sightlines should be an active upper-body physical gesture: reaching both arms high above the head, gripping the overhead pipe, and pulling downward vigorously to elevate the torso and tuck the legs.

## Decision
1. **Overhead Reach Elevation**:
   - Both hands elevated above head level ($Y_{left} > 0.22\text{ m}, Y_{right} > 0.22\text{ m}$ relative to HMD eye level).
   - Horizontal hand spacing matching shoulder width ($|\Delta X| \in [0.20, 0.55]\text{ m}$).
2. **Dual Pipe Grip Clench**:
   - Both controllers clenched with grip sensors held ($Grip_{left} > 0.60 \land Grip_{right} > 0.60$).
3. **Downward Chin-Up Pull Dynamics**:
   - Simultaneous downward arm pull velocity ($V_{y, left} < -0.50\text{ m/s} \land V_{y, right} < -0.50\text{ m/s}$) simulating hoisting body weight upward against gravity.
4. **Suspension Flex Haptics & Shadow Concealment**:
   - Synthesizes dual $75\text{ Hz}$ ($0.70$) iron pipe flex strain haptics, transitioning into a low-frequency $50\text{ Hz}$ ($0.30$) suspended grip isometric tension pulse while held overhead.
5. **Engine Input Injection**:
   - Injects jump-tuck and crouch suspension sequence (`Space` + `C`) with an $1800\text{ ms}$ hoist debounce.

## Consequences
- Provides exhilarating vertical stealth evasion directly honoring Riddick lore and film canon.
- Seamlessly transitions into death-from-above aerial drop takedowns (ADR 0033) when releasing grips while looking down at passing sentries.
