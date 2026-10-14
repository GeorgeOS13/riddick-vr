# ADR 0087: Medical Syringe Sedative Self-Injection & Bio-Adrenaline Rush

## Status
Accepted

## Context
Across Butcher Bay's infirmary wards and the Dark Athena's cryogenic laboratory quarters, emergency medical ampoules and pneumatic injectors provide critical combat stimulants, sedatives, and bio-adrenaline boosts. In the original flat title, consumption of medical items or activating stimulant canisters was initiated via menu selection or contextual interaction prompts (`E`).

For full virtual reality immersion, injecting stimulants or sedatives directly into the player's body should be performed through a natural physical gesture—bringing the pneumatic injector ampoule to the off-hand arm/shoulder or lateral neck corridor and squeezing the discharge trigger.

## Decision
1. **Spatial Injection Trajectory & Proximity**:
   - Track dominant controller position relative to the player's off-side upper arm/neck corridor:
     - Off-hand lateral offset: $X_{rel} \in [-0.24, -0.06]\text{ m}$ (for right-handed injection into left shoulder/bicep).
     - Vertical alignment: $Y_{rel} \in [-0.18, 0.12]\text{ m}$ (shoulder/neck level).
     - Forward depth: $Z_{rel} \in [-0.25, -0.05]\text{ m}$ (close to chest plane).
2. **Piston Trigger Squeeze**:
   - Require dominant hand grip held ($> 0.50$) securing the injector body, combined with dominant trigger depression ($> 0.65$) to depress the internal spring plunger.
3. **Pneumatic Discharge & Adrenaline Pulse Feedback**:
   - Piston discharge synthesizes an initial $160\text{ Hz}$ ($0.65$) sharp pneumatic skin-seal hiss ($40\text{ ms}$), followed by high-frequency needle plunge micro-haptics ($240\text{ Hz}$, $0.75$) and a resonant $100\text{ Hz}$ ($0.85$) cardiovascular bio-adrenaline rush heartbeat pulse.
4. **Game Engine Input Translation**:
   - Injects contextual medical restoration key sequence (`E` + `Q`) with a $1500\text{ ms}$ cooldown deadband to prevent duplicate ampoule consumption.

## Consequences
- Elevates medical consumable usage from passive interface clicking into tactile high-stakes survival triage.
- Seamlessly integrates with the diegetic wrist HUD, allowing players to glance at their health status and immediately jab a stimulant into their arm during intense firefights.
