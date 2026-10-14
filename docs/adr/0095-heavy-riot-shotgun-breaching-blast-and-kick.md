# ADR 0095: Heavy Riot Shotgun Breaching Hinge Blast & Kick

## Status
Accepted

## Context
Across Butcher Bay's maximum-security lockdown zones and Dark Athena containment wings, heavy reinforced steel security doors and barred security gates block progress. Riddick can breach fortified doors by placing a heavy riot shotgun muzzle point-blank against the door hinges or deadbolt locking mechanism, firing a buckshot charge, and violently kicking the door inward.

In traditional monitor gaming, door breaches were pre-canned contextual animations triggered by tapping interact (`E`). In VR, aligning the shotgun muzzle against the door lock, pulling the trigger, and driving forward with a physical boot-kick/shove motion delivers thrilling tactile agency.

## Decision
1. **Point-Blank Lock Proximity**:
   - Shotgun barrel aligned within point-blank lock contact zone ($Z_{rel} \in [-0.38, -0.16]\text{ m}, Y_{rel} \in [-0.15, 0.15]\text{ m}$).
2. **Two-Handed Blast Discharge**:
   - Active two-handed stabilization with dominant trigger depressed ($> 0.80$).
3. **Coordinated Dynamic Forward Shove / Kick**:
   - Forward thrust velocity of both arms/torso ($V_z < -1.20\text{ m/s}$) immediately following the gunshot to drive the door off its hinges.
4. **Concussive Blast & Mechanical Rupture Haptics**:
   - Synthesizes an explosive $0.98$ point-blank muzzle shockwave at $90\text{ Hz}$ in both hands, combined with an abrupt $260\text{ Hz}$ ($0.85$) shearing hinge fracture click.
5. **Engine Input Injection**:
   - Dispatches breaching sequence (`LMB` + `W` + `Space`) with an $1800\text{ ms}$ lockout buffer.

## Consequences
- Creates cinematic, tactical room entry mechanics driven entirely by natural physical gestures.
- Distinction from standard shotgun fire is guaranteed by requiring point-blank lock proximity combined with high forward thrust velocity.
