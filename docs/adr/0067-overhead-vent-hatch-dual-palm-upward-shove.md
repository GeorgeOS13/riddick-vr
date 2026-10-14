# ADR 0067: Overhead Vent Hatch Dual-Palm Upward Shove & Latch Release

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena* and *Escape from Butcher Bay*, Riddick frequently bypasses security checkpoints, locked blast doors, and guard patrols by navigating vertical ventilation shafts, maintenance shafts, and false ceilings. In desktop gameplay, opening an overhead vent hatch or ceiling access panel was reduced to aiming the crosshair upward and pressing the generic 'Use' button (`E`).

In VR, looking directly upward into an overhead ceiling hatch while pressing a flat gamepad button breaks physical immersion. Opening a spring-loaded metal conduit hatch should require the player to raise both hands overhead and execute a deliberate physical upward shove, catching the hatch rim and breaking the mechanical latches before hoisting into the shaft.

## Decision
We implement physical overhead vent hatch popping via dual-palm upward shove detection:
1. **Spatial Kinematic Envelope:**
   - Both hands raised above eye level ($Y_{local} > 0.25\text{ m}$).
   - Both hands positioned directly overhead ($Z_{local} \in [-0.25, +0.10]\text{ m}, |X_{local}| < 0.35\text{ m}$).
   - Head orientation looking upward ($\text{Pitch}_{HMD} > 0.45\text{ rad}$ / $\approx +26^\circ$).
2. **Upward Shove Velocity Filtering:**
   - Upward vertical hand velocities for both controllers exceed $+0.85\text{ m/s}$ ($V_{y, left} > +0.85\text{ m/s} \land V_{y, right} > +0.85\text{ m/s}$).
   - Grips relaxed or unclasped ($Grip < 0.60$) to simulate open palms pushing against flat sheet metal.
3. **Engine Action Dispatch & Tactile Feedback:**
   - Dispatches native interaction and climb-assist keystrokes (`E` followed by `VK_SPACE`).
   - Generates dual synchronized haptic shockwaves ($0.80$ amplitude at $160\text{ Hz}$ for $120\text{ ms}$) simulating the resonant pop of heavy sprung latch pins and the swing of heavy galvanized ductwork.

## Consequences
- Players experience an intuitive physical ceiling climb ingress.
- Upward pitch constraint ($\text{Pitch} > 0.45\text{ rad}$) prevents false triggers during boxing uppercuts or standing ladder climbs.
- Releasing spring latches feeds smoothly into the physical ledge mantling gesture (ADR 0009).
