# ADR 0106: Dark Athena Cargo Bay Magnetic Crane Remote Pendant Snatch

## Status
Accepted

## Context
In the cavernous cargo bays of the Dark Athena, massive overhead bridge cranes maneuver container stacks, armored vehicles, and freight pods. Crane consoles on the catwalks are often locked or destroyed; however, emergency push-button control pendants hang from suspended coils above the cargo floor. Riddick can leap or reach upward, snatch the hanging pendant cord, and pull it down to chest height to take remote control of the gantry hoist.

In traditional desktop play, crane operations required walking up to a fixed console trigger. In VR, looking up, reaching overhead to grab the swinging pendant, and hauling it downward provides dramatic spatial agency.

## Decision
1. **Overhead Pendant Cord Elevation**:
   - Controller elevated overhead in the crane bay corridor ($Y_{rel} > 0.18\text{ m}, Z_{rel} \in [-0.32, -0.05]\text{ m}$).
2. **Pendant Grip & Downward Haul**:
   - Controller grip sensor squeezed ($Grip > 0.55$) catching the rubberized pendant housing.
   - Downward arm pull ($V_y < -0.45\text{ m/s}$) hauling the spring-loaded coiled cable down to chest height ($Y_{rel} < 0.0\text{ m}$).
3. **Coil Cable Tension & Reluctance Haptics**:
   - Initial $60\text{ Hz}$ ($0.50$) heavy steel cable tension rumble, followed by a crisp $160\text{ Hz}$ ($0.70$) pendant relay activation click when brought to chest level.
4. **Engine Input Injection**:
   - Dispatches crane remote link sequence (`E` + `W`) with a $2000\text{ ms}$ lockout buffer.

## Consequences
- Adds dynamic verticality and physical interaction to environmental industrial navigation.
- Natural downward haul prevents false activations while aiming upward or climbing ladders.
