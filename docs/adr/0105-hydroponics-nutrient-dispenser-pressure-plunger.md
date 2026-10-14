# ADR 0105: Subterranean Hydroponics Nutrient Dispenser Pressure Plunger

## Status
Accepted

## Context
Across Butcher Bay's agricultural subterranean vaults and Dark Athena botanical vivariums, automated hydroponic nutrient lines suffer frequent pressure clogs from algae blooms and mineral crystallization. To restore irrigation flow or siphon concentrated stimulant extracts, Riddick can manually operate high-pressure brass floor pumps and accumulator plungers mounted beside grow beds.

In flat desktop gaming, restoring nutrient flow was triggered by a static interaction key (`E`). In VR, grasping the brass plunger T-handle and performing rhythmic vertical pumping strokes against hydraulic resistance delivers authentic physical immersion.

## Decision
1. **Plunger T-Handle Proximity**:
   - Controller positioned at dispenser pump elevation ($Z_{rel} \in [-0.38, -0.14]\text{ m}, Y_{rel} \in [-0.15, 0.25]\text{ m}$).
2. **Piston Grip & Downward Plunge Strokes**:
   - Controller grip sensor clenched ($Grip > 0.50$).
   - Downward stroke velocity ($V_y < -0.45\text{ m/s}$) performed across 3 consecutive cycles within $1800\text{ ms}$.
3. **Hydraulic Slosh & Pressure Relief Haptics**:
   - Each downward stroke generates $110\text{ Hz}$ ($0.60$) pressurized fluid displacement haptics, terminating on the 3rd stroke in an explosive $220\text{ Hz}$ ($0.75$) hydraulic relief valve clack.
4. **Engine Input Injection**:
   - Injects nutrient activation key sequence (`E` + `Q`) with a $2200\text{ ms}$ reset deadband.

## Consequences
- Transforms mundane environmental puzzle interactions into satisfying physical exertion.
- Temporal stroke counter guarantees deliberate player intent and eliminates accidental trigger during traversal.
