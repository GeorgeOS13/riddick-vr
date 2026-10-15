# ADR 0173: Inmate Pit Brawl Underhook Suplex & Spine Slam

## Context
In the brutal underground fighting pits of Butcher Bay, close-range grappling frequently determines life or death. When an opponent is driven against arena barriers, Riddick can secure double underhooks beneath their shoulders, hoist their entire body upward, and bridge backward into a devastating overhead suplex or downward spine slam onto the unforgiving deck grating.

## Decision
Implement physical double underhook hoist and spine slam via two-handed 6DOF combat gesture tracking:
1. **Double Underhook Hoist**:
   - Hands positioned in close infighting range:
     - $dist(L, R) < 0.25\text{ m}$.
     - Both grips clenched: $Grip_{left} > 0.65 \land Grip_{right} > 0.65$.
     - Driving both hands upward aggressively:
       - $V_{y, left} > 1.10\text{ m/s} \land V_{y, right} > 1.10\text{ m/s}$.
2. **Backward / Downward Spine Slam**:
   - Within $500\text{ ms}$ of the initial hoist:
     - Driving both arms downward/rearward with extreme force:
       - $V_{y, left} < -1.20\text{ m/s} \land V_{y, right} < -1.20\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `S` (Rearward leverage) + `Space` (Body hoist) + `C` (Deck slam) + `LMB` (Primary attack) for $220\text{ ms}$.
4. **Haptic Feedback**:
   - Dual-hand $220\text{ Hz}$ ($0.90$) underhook lift resistance shudder.
   - Dual $45\text{ Hz}$ ($0.98$) deep sub-bass spinal impact concussive shockwave.
5. **Cooldown**: $950\text{ ms}$ debounce.

## Consequences
- Adds an explosive high-impact wrestling throw to Riddick's pit brawl arsenal.
- Rewards coordinated two-handed kinetic vertical motion.
