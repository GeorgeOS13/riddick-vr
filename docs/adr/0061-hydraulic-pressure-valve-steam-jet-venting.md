# ADR 0061: Hydraulic Pressure Blast Valve Steam Jet Venting

## Status
Accepted / Implemented

## Context
Across Dark Athena's lower manifolds and Butcher Bay's subterranean refinery tanks, industrial emergency blast valves regulate hydraulic pressure. Unlike gradual manual wheel valves (ADR 0053), these quick-release levers can be struck or slammed open to unleash explosive directional steam jets that fill doorways with scalding vapor, repelling mercenary squads and triggering environmental fires.

## Decision
We implement quick-release hydraulic steam valve venting in Section 6 of `src/hook/camera_hook.cpp`:
1. **Valve Lever Zone & Kinetic Impact**:
   - Controller positioned at the valve lever:
     $Z_{local} \in [-0.40, -0.18]\text{ m}$, $Y_{local} \in [-0.20, +0.20]\text{ m}$.
   - Heavy forward strike / punch ($V_z < -1.10\text{ m/s}$) or downward lever slam ($V_y < -0.80\text{ m/s}$).
2. **Pneumatic Explosive Jet Release**:
   - Triggers instantaneous relief valve burst (`E` + `'Q'` thermal flash), venting high-velocity superheated steam into the corridor.
   - Generates sustained $130\text{ Hz}$ ($0.75$ amplitude) jet stream vibration for $400\text{ ms}$.

## Consequences
- Provides rapid reactive environmental combat options during heated firefights.
- Does not require multi-rotation valve cranking, allowing instant tactical diversions.
- Sharp kinetic threshold prevents accidental triggering when reaching near wall consoles.
