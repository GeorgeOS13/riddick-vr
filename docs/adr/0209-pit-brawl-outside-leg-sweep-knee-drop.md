# ADR 0209: Inmate Pit Brawl Outside Leg Hook Sweep & Ground Knee Drop

## Status
Accepted

## Context
In aggressive inmate brawling, Riddick exploits an opponent's high guard by hooking their lead leg laterally from the outside and driving his full body weight downward in a devastating knee drop directly onto the fallen opponent's thoracic cavity.

## Decision
Implement physical outside leg sweep and knee drop in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: From clinch or active Boxer Guard, hook opponent's outside leg with lead foot/hand ($leftLocal.y < -0.30\text{ m}, |lVel.x| > 0.90\text{ m/s}$), arming a $500\text{ ms}$ drop window.
- **Vertical Knee Drop Plunge**: Before timeout, plunge dominant fist and body downward with extreme velocity ($rVel.y < -1.25\text{ m/s}$) to crush the fallen adversary.
- **Synthesized Controls**: Emits `A`/`D` + `C` + `Space` + `S` + `LMB` for lateral sweep, deck takedown, and airborne knee-drop finish.
- **Haptic Feedback**: $200\text{ Hz}$ ($0.85$) sweep trip torque followed by $320\text{ Hz}$ ($0.98$) concussive knee-drop mat shockwave.

## Consequences
- Broadens dynamic multi-phase striking-to-grappling transitions in the pit arena.
- Pairs with mount ground-and-pound, outside trips, and double-leg slams.
