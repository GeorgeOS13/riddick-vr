# ADR 0081: Security Turret Optical Sensor Spray Paint & Smear Blind

## Status
Accepted

## Context
Automated ceiling turrets and motion-tracking security cameras restrict player movement in Butcher Bay cellblocks and Dark Athena defense corridors. In desktop play, the player had to destroy turrets with heavy ordnance or scramble past during timed sweeping rotations.

In VR, stealth agency is greatly enhanced if the player can slip close beneath a sensor dome and physically blind the tracking optics with scavenged spray paint, grease, or soot rags.

## Decision
We implement physical turret optical sensor dome blinding:
1. **Sensor Dome Proximity:**
   - Either controller raised into overhead/sensor dome height ($Y_{local} \in [0.10, 0.40]\text{ m}, Z_{local} \in [-0.35, -0.15]\text{ m}$).
   - Grip held ($Grip > 0.50$) securing an aerosol canister, grease rag, or gauntlet cloth.
2. **Horizontal Smear / Spray Sweep:**
   - Lateral sweeping motion across the lens ($|V_x| > 0.50\text{ m/s}$).
   - Cooldown buffer: $1200\text{ ms}$.
3. **Engine Action Dispatch & Aerosol Dispersion Feedback:**
   - Dispatches interaction key (`E`) and optical suppression signal (`Q`) to obscure turret tracking cameras for 15 seconds.
   - Emits sustained aerosol propellant hissing haptics on the active controller ($0.50$ amplitude at $160\text{ Hz}$ for $250\text{ ms}$).

## Consequences
- Expands non-lethal stealth infiltration options in heavily guarded corridors.
- Distinct spatial envelope prevents conflict with two-handed aiming and boxing guard stances.
