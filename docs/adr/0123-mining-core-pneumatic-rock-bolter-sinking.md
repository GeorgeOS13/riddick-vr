# ADR 0123: Mining Core Pneumatic Rock Bolter Sinking & Anchor Driver

## Status
Accepted

## Context
In Butcher Bay's lower mining sectors and unstable bedrock excavation veins, falling granite slabs and overhead collapse hazards require stabilizing loose tunnel arches with pneumatic expansion rock bolters. Riddick can also drive high-tension structural anchor bolts to secure rappelling lines across chasm pits.

## Decision
1. **Two-Handed Guide Sleeve & Handle Alignment**:
   - Off-hand positioned against rock face / guide barrel collar ($Z \in [-0.45, -0.16]\text{ m}, Y \in [-0.20, 0.25]\text{ m}$, $Grip_{left} > 0.60$).
   - Dominant hand aligned behind tool handle ($dist(Left, Right) \in [0.18, 0.40]\text{ m}$, $Grip_{right} > 0.60$).
2. **Kinetic Ramming & Pneumatic Driver Stroke**:
   - Dominant hand drives forward into insertion sleeve ($V_{z, right} < -1.10\text{ m/s}$) while holding trigger ($Trigger_{right} > 0.65$).
3. **Pneumatic Percussion & Mechanical Torque-Lock Haptics**:
   - Off-hand: Continuous high-frequency sleeve rattle ($210\text{ Hz}$, $0.65$, $120\text{ ms}$).
   - Dominant hand: Heavy pneumatic reciprocating piston impact pulses ($55\text{ Hz}$, $0.98$, $160\text{ ms}$) terminating in a solid anchor torque-lock clunk.
4. **Engine Input Injection**:
   - Dispatches structural anchoring sequence (`E` + `Space` + `LMB`) with a $1500\text{ ms}$ lockout.

## Consequences
- Expands subterranean mining interactions with heavy two-handed pneumatic anchor tooling.
- Seamlessly fits two-handed industrial interaction paradigms established in ADRs 0037, 0053, and 0113.
