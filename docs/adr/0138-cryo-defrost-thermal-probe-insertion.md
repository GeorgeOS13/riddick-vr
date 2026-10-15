# ADR 0138: Cryo-Chamber Defrost Temperature Sensor Probe Insertion

## Status
Accepted

## Context
In Butcher Bay's cryogenic holding modules, unfreezing locked stasis capsules without proper thermal profiling risks shattering internal specimens or triggering emergency incinerator overrides. Riddick inserts an off-hand diagnostic temperature probe into the pod's thermal port and holds it steady to calibrate the rapid defrost cycle.

## Decision
1. **Umbilical Thermal Port Insertion**:
   - Controller positioned at the diagnostic thermal port ($Z \in [-0.38, -0.14]\text{ m}, Y \in [-0.15, 0.20]\text{ m}$).
   - Grip held firmly ($Grip > 0.55$).
2. **Stable Diagnostic Calibration Dwell**:
   - Motion velocity constrained within fine limits:
     - $|\vec{V}| < 0.12\text{ m/s}$.
   - Position maintained continuously for $\ge 400\text{ ms}$.
3. **Ascending Thermal Equilibrium & Defrost Chime Haptics**:
   - Initial needle contact tap ($250\text{ Hz}$, $0.40$, $30\text{ ms}$).
   - Ascending calibration drone ($110\text{ Hz} \to 220\text{ Hz}$, $0.30 \to 0.70$).
   - Defrost authorization chime on completion ($280\text{ Hz}$, $0.80$, $90\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches thermal defrost calibration sequence (`E` + `Tab` + `Q`) with a $1500\text{ ms}$ lockout.

## Consequences
- Deepens interactive cryo-facility navigation alongside biometric bypasses (ADR 0097) and defrost keypads (ADR 0112).
- Rewards fine motor control and physical stability in atmospheric puzzle rooms.
