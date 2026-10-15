# ADR 0137: Dark Athena EVA Solar Array Cable Tension Winch Crank

## Status
Accepted

## Context
During deep space spacewalks along the exterior chassis of the Dark Athena, micrometeoroid impacts can sever or slacken structural guy-wires supporting the deployable solar arrays. Riddick must physically turn the manual tension winch spindle to take up cable slack, locking the solar arrays in their optimum angle.

## Decision
1. **Winch Spindle Contact Zone**:
   - Controller positioned at the winch hub ($Z \in [-0.42, -0.15]\text{ m}, Y \in [-0.25, 0.15]\text{ m}$).
   - Grip held firmly ($Grip > 0.55$).
2. **Vertical Circular Cranking Revolutions**:
   - Tracks circular rotation in the pitch plane ($|\vec{V}_{yz}| > 0.48\text{ m/s}$).
   - Requires accumulating 3 full circular cranking revolutions ($\ge 1080^\circ$).
3. **Ratchet Pawl & Cable Tension Lock Haptics**:
   - Progressive ratchet tooth clicks ($140\text{ Hz}$, ascending from $0.55$ to $0.85$ per turn).
   - Solid mechanical dog-lock snap and singing cable resonance on completion ($90\text{ Hz}$, $0.90$, $120\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches solar cable winch tensioning command (`E` + `W` + `Enter`) with a $1500\text{ ms}$ lockout.

## Consequences
- Further enriches EVA exterior maintenance gameplay alongside solar array cranking (ADR 0114), antenna polarization (ADR 0125), and weld inspection (ADR 0131).
- Provides tactile mechanical feedback simulating heavy steel cable tension under zero-gravity conditions.
