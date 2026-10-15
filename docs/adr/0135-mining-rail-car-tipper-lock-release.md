# ADR 0135: Subterranean Mining Rail Pneumatic Car Tipper Lock Release

## Status
Accepted

## Context
In Butcher Bay's lower mine shafts and slag disposal chutes, heavy ore rail cars must be dumped to create barricades, clear obstructed tracks, or dump molten slag onto pursuing riot guards. The pneumatic car tipper uses a dual-handled locking dog clamp at the chassis pedestal that requires an explosive two-handed upward hoist to trip.

## Decision
1. **Low Dual-Handed Pedestal Grip**:
   - Both hands reach down to the chassis locking dogs ($Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.55, -0.20]\text{ m}$).
   - Both grips clenched securely ($Grip_{left} > 0.60 \land Grip_{right} > 0.60$).
2. **Explosive Upward Hoist Stroke**:
   - Both hands yank upward simultaneously with high velocity:
     - $V_{y, left} > 0.90\text{ m/s} \land V_{y, right} > 0.90\text{ m/s}$.
3. **Locking Pawl Release & Ore Tipper Tilt Haptics**:
   - Dual-hand mechanical pawl release snap ($100\text{ Hz}$, $0.85$, $60\text{ ms}$).
   - Followed by deep pneumatic valve discharge and chassis tilt rumble ($50\text{ Hz}$, $0.95$, $200\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches ore car dump sequence (`E` + `Space` + `Enter`) with a $1500\text{ ms}$ lockout.

## Consequences
- Expands subterranean rail operations alongside mining cart handbrakes (ADR 0048), track switches (ADR 0076), and turntables (ADR 0108).
- Gives players a dynamic environmental hazard to trigger in combat and exploration.
