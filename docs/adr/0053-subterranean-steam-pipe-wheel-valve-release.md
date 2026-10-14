# ADR 0053: Subterranean High-Pressure Steam Pipe Wheel Valve Release

## Status
Accepted / Implemented

## Context
Butcher Bay's subterranean mining depths and Dark Athena's lower engine works feature networks of pressurized superheated steam conduits. In classic stealth encounters, Riddick can manipulate industrial wheel valves to vent scalding steam across corridors, obscuring enemy thermal/visual sightlines, triggering environmental diversion alarms, and scalding unaware guards.

In virtual reality, turning a flat industrial handwheel using a single button press feels detached from the tactile reality of heavy mechanical machinery. Players should reach out, grasp the valve rim, rotate their wrist, and feel escalating pneumatic tension until the valve blows.

## Decision
We implement high-pressure steam valve manipulation in Section 6 of `src/hook/camera_hook.cpp`:
1. **Valve Contact & Grip Verification**:
   - Controller positioned in the valve zone:
     $Z_{local} \in [-0.42, -0.18]\text{ m}$, $Y_{local} \in [-0.10, +0.30]\text{ m}$.
   - Controller grip engaged ($> 0.55$).
2. **Tangential Revolution Tracking**:
   - Evaluates wrist angular/tangential velocity:
     $V_{\text{tangential}} = \sqrt{V_x^2 + V_y^2} > 0.45\text{ m/s}$.
   - Each qualifying rotation pulse increments valve displacement counter `s_steamValveRotations` (debounced to $220\text{ ms}$) with $160\text{ Hz}$ ($0.50$ amplitude) metal-on-metal friction ticks.
3. **Pneumatic Scalding Steam Release**:
   - Upon completing 3 full rotations, the relief valve pops:
     Dispatches interaction impulse (`E` + `Q` thermal diversion), venting superheated vapor.
   - Emits sustained $110\text{ Hz}$ ($0.70$ amplitude) pneumatic vibration across the interacting hand, accompanied by environmental steam hiss.
   - Resets rotation counter after a $1200\text{ ms}$ cooldown.

## Consequences
- Provides physical mechanical agency for environmental sabotage and tactical stealth diversions.
- Multi-stroke requirement creates tension while working near patrolling enemy sentries.
- Distinct vibration frequency ($110\text{ Hz}$) clearly distinguishes steam release from electrical or bulkhead events.
