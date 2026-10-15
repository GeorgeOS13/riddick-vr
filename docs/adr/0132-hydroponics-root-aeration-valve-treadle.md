# ADR 0132: Subterranean Hydroponics Root Aeration Valve Foot Pedal Treadle

## Status
Accepted

## Context
Butcher Bay's subterranean hydroponics cultivation trenches utilize heavy cast-iron foot treadles to pressurize deep-root aeration accumulators. When electrical manifolds fail or circuit breakers are blown, manual treadle pumping provides the necessary pressure to displace stagnant acidic liquid and expose bypass hatches.

## Decision
1. **Low Crouched Stance**:
   - Player physically crouched or low to the deck ($Y_{HMD} < -0.30\text{ m} \lor Y_{local} < -0.45\text{ m}$).
2. **Pneumatic Treadle Plunge**:
   - Controller driven vertically downward toward floor grating ($V_y < -0.95\text{ m/s}, Y < -0.48\text{ m}$).
   - Controller gripped securely ($Grip > 0.50$).
3. **Cast-Iron Spring Compression & Pneumatic Surge Haptics**:
   - Heavy mechanical spring compression pulse ($75\text{ Hz}$, $0.65$, $50\text{ ms}$).
   - Followed by high-pressure fluid aeration surge rumble ($120\text{ Hz}$, $0.90$, $180\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches aeration treadle pump command (`E` + `C` + `Enter`) with a $1500\text{ ms}$ lockout.

## Consequences
- Broadens physical floor-level interactions while crouched in agricultural and subterranean utility spaces.
- Interlocks cleanly with existing physical crouch mechanics (ADR 0025, ADR 0030).
