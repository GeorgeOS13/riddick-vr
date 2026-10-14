# ADR 0112: Cryo-Chamber Defrost Thermal Cycle Override Keypad Punch

## Status
Accepted

## Context
In the Butcher Bay Cryo Facility, frozen prisoner pods remain locked in stasis until a rapid thermal defrost cycle is engaged. Players must interact with the wall-mounted membrane keypad next to each pod by executing physical fingertip taps to enter the thermal bypass sequence.

## Decision
1. **Keypad Surface Interaction Zone**:
   - Hand within interactive terminal plane ($Z_{local} \in [-0.38, -0.15]\text{ m}, Y_{local} \in [-0.15, 0.20]\text{ m}, |X_{local}| < 0.22\text{ m}$).
   - Controller index finger extended (Trigger $< 0.20$ and Grip $< 0.35$).
2. **Tactile Membrane Poke Gesture**:
   - Forward finger strike into wall plane ($V_z < -0.32\text{ m/s}$).
   - Requires 3 consecutive keypad taps within a $1200\text{ ms}$ window to complete the defrost sequence.
3. **Membrane Keypad Click Haptics**:
   - Sharp, responsive tactile clicks ($260\text{ Hz}$, $0.45$, $35\text{ ms}$) on each contact.
   - Long confirmation chime ($180\text{ Hz}$, $0.70$, $100\text{ ms}$) upon the 3rd successful strike.
4. **Engine Input Injection**:
   - Dispatches terminal defrost sequence (`E` + `Enter` + `Q`) to initiate rapid chamber thawing.

## Consequences
- Elevates terminal interactions from simple abstract button presses to physical, diegetic spatial inputs.
- Distinct from palm presses (NanoMED, ADR 0019) through strict trigger/grip openness checks.
