# ADR 0076: Subterranean Mining Cart Rail Track Switch Wrench Pull

## Status
Accepted

## Context
In the lower mining facility beneath Butcher Bay and Dark Athena's cargo sorting rails, ore rail cars traverse multi-track routing switches. Navigating the track maze requires throwing heavy mechanical track switch handles to divert cars away from crushing hazards and toward maintenance docks.

In the original game, track switches were operated by standard `E` activation. In VR, wrenching heavy mechanical rail switches should require physical leverage and lateral throw.

## Decision
We implement physical mining rail switch lever throwing:
1. **Switch Handle Proximity Envelope:**
   - Dominant controller positioned at ground/wall rail switch level ($Y_{local} \in [-0.55, -0.15]\text{ m}, Z_{local} \in [-0.42, -0.16]\text{ m}$).
   - Controller grip clutched ($Grip > 0.50$).
2. **Lateral Wrench Throw Kinematics:**
   - Rapid lateral throw across the track axis ($|V_x| > 0.70\text{ m/s}$).
   - Cooldown buffer: $850\text{ ms}$.
3. **Action Dispatch & Heavy Steel Rail Clank Feedback:**
   - Dispatches interaction key (`E`) to throw rail turnout points.
   - Generates heavy iron rail clank and ground shudder haptics on the throwing controller ($0.85$ amplitude at $70\text{ Hz}$ for $180\text{ ms}$) simulating the shifting of solid steel guide rails.

## Consequences
- Elevates industrial environmental navigation into a visceral physical action.
- Low height envelope ($Y < -0.15\text{ m}$) ensures separation from eye-level combat gestures.
