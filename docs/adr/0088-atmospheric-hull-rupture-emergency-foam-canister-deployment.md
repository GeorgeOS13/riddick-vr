# ADR 0088: Atmospheric Hull Rupture Emergency Foam Canister Deployment

## Status
Accepted

## Context
During exterior hull EVA breaches and micro-meteoroid decompression emergencies aboard the Dark Athena, structural fissures vent cabin atmosphere into the vacuum of space. Handheld emergency sealant foam canisters allow repair crews and infiltrators to seal compromised seam ruptures before structural integrity cascades into explosive decompression.

In standard monitor play, sealant operations relied on pre-scripted cutscenes or stationary interaction button presses (`E`). In VR, deploying rapid-curing chemical foam across a ruptured seam requires two-handed canister stabilization, nozzle trajectory guidance, and continuous trigger discharge while sweeping along the fracture line.

## Decision
1. **Canister Two-Handed Stabilization**:
   - Both hands brought into proximity ($dist < 22\text{ cm}$) with off-hand grip held ($> 0.55$) supporting the nozzle barrel and dominant grip held ($> 0.55$) clenching the pressurized canister cylinder.
2. **Breach Proximity & Trajectory**:
   - Nozzle positioned within breach repair zone ($Z_{rel} \in [-0.44, -0.16]\text{ m}, Y_{rel} \in [-0.30, 0.20]\text{ m}$).
   - Both triggers depressed ($> 0.60$) to open dual chemical prepolymer valves.
3. **Seam Sweeping Dynamics**:
   - Lateral sweeping velocity across breach fissure ($|V_x| > 0.30\text{ m/s}$ or $|V_y| > 0.30\text{ m/s}$) extrudes expanding acoustic sealant foam.
4. **Extrusion & Rapid Cure Haptics**:
   - Synthesizes sustained $120\text{ Hz}$ ($0.60$) pressurized aerosol jet vibration in dominant hand, coupled with $95\text{ Hz}$ nozzle expansion rumble in off-hand, terminating in a sharp $200\text{ Hz}$ ($0.75$) polymerization cure pop.
5. **Engine Input Injection**:
   - Dispatches emergency patch sequence (`E` + `Space`) with a $2000\text{ ms}$ latch deadband.

## Consequences
- Transforms depressurization set-pieces into intense, hands-on spatial hazard containment challenges.
- Prevents accidental triggering during general firearm handling by enforcing two-handed dual-trigger depression combined with lateral nozzle sweeping across the fissure.
