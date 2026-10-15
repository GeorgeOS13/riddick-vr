# ADR 0156: Subterranean Hydroponics Bio-Waste Incinerator Burner Ignition Spark Wheel Strike

## Status
Accepted

## Context
Following the manual tripping of the incinerator blast door levers (ADR 0152), the fuel-air mixture in the combustion chamber requires ignition. The automatic electronic ignition systems in the derelict lower levels are corroded, requiring Riddick to manually rotate a heavy ferrocerium spark wheel using a sharp kinetic strike.

In VR, physical flint-wheel strikes provide satisfying mechanical interaction and visual-haptic payoff.

## Decision
We implement **Subterranean Hydroponics Bio-Waste Incinerator Burner Ignition Spark Wheel Strike**:
1. **Spark Wheel Corridor**:
   - Dominant hand positioned at the combustion manifold ignition port ($Z \in [-0.40, -0.16]\text{ m}$, $Y \in [-0.15, 0.20]\text{ m}$).
   - Dominant grip held ($> 0.55$) in striking posture.
2. **Tangential Strike Impulse**:
   - Executing a sharp downward tangential strike across the wheel face ($V_y < -0.90\text{ m/s}$) spins the abrasive rotor against the flint block.
3. **Input Dispatch**:
   - Dispatches `E` (interact) + `Tab` (manifold sequence) + `Enter` (ignition cycle).
4. **Haptic Feedback**:
   - High-friction $260\text{ Hz}$ ($0.70$) flint rasp followed immediately by a deep $60\text{ Hz}$ ($0.95$) combustion flame front blast.

## Consequences
- Authentic industrial puzzle progression tied to the incinerator system.
- Fully debounced with a $1500\text{ ms}$ reset window.
