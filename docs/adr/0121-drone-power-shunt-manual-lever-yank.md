# ADR 0121: Cybernetic Drone Power Shunt Manual Lever Yank

## Status
Accepted

## Context
Across the Dark Athena's drone production bays and repair berths, deactivated cybernetic drones and charging alcoves house volatile high-capacity capacitor banks. By reaching into the drone's lateral chassis service hatch and violently snapping the manual power shunt lever downward, Riddick can overload the drone's power bus, generating a localized EMP blast that disables surrounding electronic door locks and optic sensors.

## Decision
1. **Chassis Service Hatch Zone**:
   - Controller positioned at drone side access hatch ($Z_{local} \in [-0.42, -0.14]\text{ m}, Y_{local} \in [-0.15, 0.20]\text{ m}$).
   - Grip clenched firmly ($Grip > 0.50$).
2. **Downward Shunt Breaker Yank**:
   - Downward throw gesture: $V_y < -0.60\text{ m/s}$.
3. **Electro-Mechanical Arc Discharge Haptics**:
   - Crisp spring-loaded breaker switch snap ($200\text{ Hz}$, $0.60$, $40\text{ ms}$).
   - Followed immediately by violent inductive discharge shudder ($280\text{ Hz}$, $0.90$, $110\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches shunt overload sequence (`E` + `Space` + `Q`) with a $2000\text{ ms}$ reset deadband.

## Consequences
- Expands stealth puzzle navigation through environmental electronic overrides.
- Complements drone umbilical disconnection (ADR 0094) and drone chassis snap (ADR 0058).
