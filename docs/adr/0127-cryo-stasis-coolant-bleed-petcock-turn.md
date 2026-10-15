# ADR 0127: Cryo-Stasis Chamber Manual Coolant Bleed Petcock Turn

## Status
Accepted

## Context
In Butcher Bay's cryo-stasis containment wings and cryogenic processing nodes, high-pressure liquid nitrogen feeds maintain sub-zero temperatures. Riddick can manually unseat brass bleed petcocks to vent localized supercooled aerosol plumes, quenching thermal imaging sweeps and blinding security camera grids.

## Decision
1. **Coolant Line Petcock Contact**:
   - Controller positioned in proximity to the line valve spindle ($Z \in [-0.40, -0.15]\text{ m}, Y \in [-0.30, 0.10]\text{ m}$).
   - Grip held firmly ($Grip > 0.55$).
2. **Sharp Angular Radial Wrist Turn**:
   - Requires a rapid radial twist or horizontal shear ($|\Delta\text{roll}| > 1.2\text{ rad} \lor |V_x| > 0.55\text{ m/s}$).
3. **Crystalline Frost Snap & Shivering Aerosol Haptics**:
   - Initial brittle thread fracture detent ($260\text{ Hz}$, $0.60$, $30\text{ ms}$).
   - Transitioning into sustained high-velocity refrigerant hiss rumble ($140\text{ Hz}$, $0.70$, $180\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches cryogenic coolant bleed command (`E` + `Space` + `Enter`) with a $1500\text{ ms}$ lockout.

## Consequences
- Provides tactile control over cryo-cooling infrastructure, offering environmental cover against thermal vision and turret sensor sweeps.
- Pairs with cryo-glass frost wiping (ADR 0051) and nitrogen coolant valve strikes (ADR 0074).
