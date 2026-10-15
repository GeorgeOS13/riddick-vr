# ADR 0161: Inmate Pit Brawl Upward Palm Chin-Strike & Cervical Snap

## Context
In the inmate brawl pits of Butcher Bay and the slave pens of the *Dark Athena*, close-quarters combat frequently degenerates into brutal infighting where conventional punches are jammed. A classic lethal technique involves driving an open palm upward under an opponent's chin to hyperextend their neck, immediately followed by grasping the jaw and skull to execute a catastrophic lateral cervical twist, incapacitating or eliminating the opponent instantly.

## Decision
Implement physical upward palm chin-strike followed by rapid cervical snap via 6DOF combat gesture tracking:
1. **Primary Upward Palm Strike**:
   - Starting from Boxer Guard or close infighting range:
     - Dominant hand drives explosively upward and forward:
       - $V_{y, dominant} > 1.25\text{ m/s}$ (rapid upward thrust).
       - $V_{z, dominant} < -0.80\text{ m/s}$ (forward penetration).
       - $Y_{controller} \in [-0.10, 0.25]\text{ m}$ (striking chin/facial level).
2. **Immediate Lateral Cervical Snap**:
   - Within a $500\text{ ms}$ combo window following the upward strike:
     - Off-hand executes a violent lateral neck snap whip ($|V_{x, off}| > 1.10\text{ m/s}$).
3. **Synthesized Game Controls**:
   - Synthesizes `W` (Forward step) + `LMB` (Primary attack) + `Space` (Takedown leverage) for $160\text{ ms}$.
4. **Haptic Feedback**:
   - Dominant hand receives $240\text{ Hz}$ ($0.98$) mandibular percussion impact blast.
   - Off-hand receives $300\text{ Hz}$ ($0.98$) cervical bone snap fracture haptic shudder.
5. **Cooldown**: $850\text{ ms}$ debounce.

## Consequences
- Expands Riddick's barehanded pit brawl repertoire with a lethal close-range two-handed combo.
- Perfectly complements existing guard counters, shin stomps, liver hooks, and headbutts.
