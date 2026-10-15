# ADR 0146: Security Guard Stun Baton Charged Underarm Spear Thrust

## Status
Accepted

## Context
When facing aggressive riot guards with ballistic shields or heavy cybernetic assault drones, wide swings with the stun baton can bounce off reinforced armor or expose Riddick to counter-strikes. Riddick can brace the baton under his forearm and drive a direct, high-velocity underarm spear thrust directly into an enemy's throat notch, solar plexus, or unarmored joint gaps while holding the trigger to discharge the full capacitance bank on impact.

In VR, physical two-handed spear thrusting provides a distinct tactical choice compared to downward slams (ADR 0077) or rib-jabs (ADR 0133).

## Decision
We implement **Security Guard Stun Baton Charged Underarm Spear Thrust**:
1. **Underarm Spear Grip & Bracing**:
   - Dominant hand gripping baton with trigger held ($Grip > 0.60 \land Trigger > 0.65$).
   - Off-hand positioned near dominant wrist/forearm to stabilize thrust vector ($dist(Left, Right) < 0.22\text{ m}$).
2. **Dynamic Spear Thrust**:
   - Driving the baton straight forward at high velocity ($V_{dominant, z} < -1.35\text{ m/s}$).
3. **Input Dispatch**:
   - Dispatches `W` (forward surge) + `S` (ground plant) + `LMB` (primary attack).
4. **Haptic Feedback**:
   - Dominant hand delivers a lethal $0.98$ linear electro-kinetic impact shockwave at $310\text{ Hz}$, while the stabilizing off-hand receives a $0.65$ bracing recoil pulse at $140\text{ Hz}$.

## Consequences
- Adds an aggressive linear armor-piercing option to the stun baton arsenal.
- $1500\text{ ms}$ cooldown ensures balanced combat flow.
