# ADR 0074: Subterranean Cryo-Nitrogen Coolant Valve Venting Diversion

## Status
Accepted

## Context
In deep industrial and cryogenic storage sectors, high-pressure liquid nitrogen cooling conduits run along service catwalks. In flat desktop gameplay, ambient environmental puzzles often required Riddick to turn rusted valves to vent hazards or create diversions.

In VR, striking or wrenching a frost-encrusted nitrogen wheel valve to crack brittle ice dams and vent sub-zero thermal plumes transforms static industrial scenery into an active tactical asset.

## Decision
We implement physical cryo-nitrogen coolant valve fracturing and venting:
1. **Spatial Proximity & Contact Envelope:**
   - Dominant controller positioned at valve station ($Z_{local} \in [-0.42, -0.16]\text{ m}, Y_{local} \in [-0.20, +0.25]\text{ m}$).
   - High-velocity blunt strike or wrenching stroke ($|V| > 1.05\text{ m/s}$ or rapid angular twist $|V_x| > 0.80\text{ m/s}$).
   - Grip engaged ($Grip > 0.50$).
2. **Ice Fracture & Thermal Valve Release:**
   - Cooldown debounce: $1200\text{ ms}$.
   - Dispatches environmental interaction and diversion signal keys (`E` + `Q`).
3. **Crystalline Fracture & Supercooled Fog Haptics:**
   - Emits a brittle ice-shattering crack haptic pulse on impact ($0.85$ amplitude at $260\text{ Hz}$ for $120\text{ ms}$) followed by sustained sub-zero cryo-exhaust turbulence ($0.60$ at $130\text{ Hz}$).

## Consequences
- Deepens interactive environmental tactics against pursuers and hostile patrols.
- High velocity threshold ($|V| > 1.05\text{ m/s}$) prevents accidental activation during cautious movement.
