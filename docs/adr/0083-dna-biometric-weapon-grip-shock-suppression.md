# ADR 0083: Security Guard DNA Biometric Weapon Grip Shock Suppression

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*, security personnel carry firearms equipped with genetic DNA biometrics. If unauthorized inmates (or Riddick) grasp an encoded guard assault rifle without biometric authorization or specialized insulated gauntlet dampening, the weapon discharges an agonizing electrostatic shock through the grip to force the user to drop it.

In 2D desktop gameplay, this was represented by a simple health-drain ticking sound and a screen flash. In VR, tactile feedback can make biometric security rejection intensely physical through escalating electro-haptic shocks and responsive physical drop mechanics.

## Decision
We implement physical DNA biometric weapon grip shock and insulation suppression:
1. **Biometric Shock State Machine:**
   - When holding an unauthorized firearm (primary weapon slot '1') without insulation suppression:
     - Emits escalating electrostatic high-frequency shock pulses through the dominant controller grip ($0.40 \to 0.85$ amplitude at $250\text{ Hz}$).
   - **Emergency Weapon Drop:**
     - Relaxing the dominant grip ($Grip < 0.25$) immediately drops the firearm to the deck (`G` / switch to fists '3'), extinguishing the shock.
2. **Off-Hand Insulated Gauntlet Suppression Gesture:**
   - Bringing the off-hand to wrap around the dominant grip hand ($dist_{hands} < 0.15\text{ m}$) with off-hand grip held ($Grip_{left} > 0.60$) simulates wrapping insulated carbon-fiber tape or gauntlet cloth around the biometric receiver.
   - Suppresses the DNA circuit (`E`), converting the shock into a neutralized low hum ($0.20$ at $80\text{ Hz}$).

## Consequences
- Deepens tactical tension when scavenging high-tier guard weaponry.
- Natural physical reflex (dropping the weapon when shocked) immediately resolves danger without menu navigation.
