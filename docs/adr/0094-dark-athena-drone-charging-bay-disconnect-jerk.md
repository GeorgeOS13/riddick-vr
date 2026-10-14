# ADR 0094: Dark Athena Drone Charging Bay Station Disconnect Jerk

## Status
Accepted

## Context
Aboard the Dark Athena, cybernetic attack drones are stored and recharged in vertical wall alcoves connected via heavy industrial high-voltage umbilicals. Infiltrating drone recharge stations allows Riddick to sabotage power conduits, scramble station security logic, or hot-jack into drone telemetry units for cybernetic remote piloting.

In flat desktop gameplay, unhooking drones or sabotaging charging bays was represented by standard contextual interact triggers (`E`). In VR, physically grappling high-voltage umbilical connectors and violently ripping them free from docking receptacles delivers maximum visceral impact.

## Decision
1. **Recharge Alcove Proximity**:
   - Reaching either hand into the docking bay receptacle zone:
     - Depth forward: $Z_{rel} \in [-0.44, -0.16]\text{ m}$.
     - Vertical height: $Y_{rel} \in [0.05, 0.38]\text{ m}$ (chest/head level in alcove).
2. **Coupling Clamp & Violent Rearward Jerk**:
   - Hand grip clenched firmly ($Grip > 0.60$) grabbing the armored power connector.
   - Violent rearward yank velocity ($V_z > 0.85\text{ m/s}$) breaking magnetic and mechanical detent catches.
3. **High-Voltage Arc & Detent Snapping Haptics**:
   - Synthesizes explosive $240\text{ Hz}$ ($0.90$) inductive arc discharge and high-voltage cable shudder ($70\text{ ms}$) in the pulling hand, followed by $110\text{ Hz}$ mechanical recoil.
4. **Engine Input Injection**:
   - Dispatches emergency decouple sequence (`E` + `Space`) with a $1500\text{ ms}$ reset threshold.

## Consequences
- Elevates drone sabotaging and hijacking encounters into high-voltage tactile set-pieces.
- Rearward pull trajectory prevents accidental trigger while aiming or punching forward.
