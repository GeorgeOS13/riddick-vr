# ADR 0100: Dark Athena Fusion Reactor Core Rod Manual Insertion

## Status
Accepted

## Context
Aboard the Dark Athena, the vessel's primary antimatter/fusion warp drive requires manual magnetic containment fuel rod alignment to initialize main thrusters or trigger drive core overloads. In standard desktop play, key story sequences like reactor rod insertion were relegated to cutscenes or fixed camera interaction points.

For the historic 100th Architecture Decision Record of the Riddick VR remake, we formalize the ultimate bimanual heavy industrial mechanism: physically grasping a massive glowing magnetic containment rod with both hands, hoisting it above the shoulder, and violently plunging it down into the core containment channel against heavy magnetic repulsor forces.

## Decision
1. **Bimanual Fuel Rod Grip**:
   - Both hands brought into heavy tool proximity ($dist < 32\text{ cm}$) with both grips clenched firmly ($Grip_{left} > 0.60 \land Grip_{right} > 0.60$).
2. **Overhead Alignment & Core Channel Plunge**:
   - Rod hoisted above chest plane ($Y_{left} > 0.15\text{ m} \land Y_{right} > 0.15\text{ m}$).
   - Coordinated violent forward and downward insertion plunge ($V_z < -1.15\text{ m/s} \land V_y < -0.80\text{ m/s}$).
3. **Seismic Magnetic Containment & Core Lock Haptics**:
   - Initial magnetic repulsion resistance ($180\text{ Hz}$ buzzing repulsion field, $0.65$) building up to the insertion point.
   - Core seat detonation generates a catastrophic $45\text{ Hz}$ ($0.98$) deep sub-bass structural shockwave across both controllers for $250\text{ ms}$, terminating in dual $280\text{ Hz}$ ($0.85$) pneumatic locking latch clicks.
4. **Engine Input Injection**:
   - Dispatches reactor critical command sequence (`E` + `Space` + `VK_RETURN`) with a $3000\text{ ms}$ latch deadband.

## Consequences
- Commemorates the 100th system milestone with a monumental, visceral climax interaction.
- Provides deep tactile immersion for the game's most intense narrative and engineering set-pieces.
