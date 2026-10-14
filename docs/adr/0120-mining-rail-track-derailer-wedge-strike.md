# ADR 0120: Subterranean Mining Rail Track Derailer Wedge Strike

## Status
Accepted

## Context
In the deep subterranean mining levels of Butcher Bay, armed guards and autonomous mining drones patrol along rail lines using motorized ore haulers. To neutralize pursuing rail vehicles or derail armored cars, Riddick can plant an iron derailer shoe wedge onto the rail crown and hammer it into place using a downward tool strike.

## Decision
1. **Low Rail Crown Stance**:
   - Player physically crouched near track bed ($Y_{HMD} < -0.30\text{ m}, Y_{local} < -0.45\text{ m}$).
   - Off-hand holding derailer wedge against the rail ($Z_{local, left} \in [-0.45, -0.16]\text{ m}, Grip_{left} > 0.55$).
2. **Kinetic Hammer Strike Gesture**:
   - Dominant hand driving heavy tool/pipe downward violently into the wedge:
     - $V_{y, right} < -1.10\text{ m/s}$.
3. **Anvil Strike & Rail Resonance Haptics**:
   - Dominant hand: Sharp metallic anvil crack ($260\text{ Hz}$, $0.95$, $50\text{ ms}$).
   - Off-hand: Deep resonant rail ring ($85\text{ Hz}$, $0.60$, $140\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches track sabotage sequence (`E` + `Space` + `Tab`) with an $1800\text{ ms}$ lockout.

## Consequences
- Expands rail interaction beyond turntables (ADR 0108) and switches (ADR 0076) into aggressive tactical sabotage.
- Natural two-handed positioning prevents accidental triggers while walking over tracks.
