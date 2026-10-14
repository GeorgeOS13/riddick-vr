# ADR 0111: Security Guard Riot Shield Wedge Pry & Disarm

## Status
Accepted

## Context
Heavily armored Butcher Bay guards advance behind ballistic riot shields, blocking standard frontal melee strikes. While parrying and weapon stripping exist (ADR 0018, ADR 0063), facing a deployed riot shield requires driving a melee weapon or shiv into the shield's lateral bracket seam and wrenching it away with bilateral leverage.

## Decision
1. **Shield Seam Wedge Engagement**:
   - Both hands brought forward into shield perimeter zone ($Z_{local} \in [-0.42, -0.16]\text{ m}, Y_{local} \in [-0.20, 0.20]\text{ m}$).
   - Off-hand grasping shield rim ($Grip_{left} > 0.60$) while dominant hand drives weapon into bracket ($Trigger_{right} > 0.50 \lor Grip_{right} > 0.60$).
2. **Violent Outward Wrench Leverage**:
   - Bilateral opposing outward torque gesture:
     - Left controller jerked outward/downward: $V_{x, left} < -0.90\text{ m/s} \lor V_{y, left} < -0.80\text{ m/s}$.
     - Dominant controller pried outward/upward: $V_{x, right} > 0.90\text{ m/s} \lor V_{y, right} > 0.80\text{ m/s}$.
3. **Prying & Disarm Haptics**:
   - Initial high-frequency friction groan ($190\text{ Hz}$, $0.50$).
   - Concluding metallic shear crack ($280\text{ Hz}$, $0.95$) as the shield is ripped from the guard's arm.
4. **Engine Input Injection**:
   - Injects disarm sequence (`E` + `LMB` + `1`) with an $1800\text{ ms}$ lockout.

## Consequences
- Gives the player an aggressive, physical counter-measure against shielded security squads.
- Requires coordinated two-handed force, eliminating accidental triggers during standard punching.
