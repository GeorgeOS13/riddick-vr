# ADR 0185: Inmate Pit Brawl Standing Arm-Triangle Choke & Tracheal Clamp

## Status
Accepted

## Context
In close-range inmate pit brawling against aggressive punchers, slipping an incoming hook enables Riddick to wrap his lead arm around the opponent's neck and trap their shoulder against their carotid artery, locking a standing arm-triangle choke and squeezing until the adversary blackouts.

## Decision
Implement physical standing arm-triangle choke in `src/hook/camera_hook.cpp`:
- **Grapple Entry**: From active Boxer Guard or clinch range ($dist < 0.25\text{ m}$), drive lead arm across opponent's neck ($V_z < -0.80\text{ m/s}, |V_x| > 0.75\text{ m/s}$, Left Grip $> 0.65$).
- **Bicep Lock & Compression Squeeze**: Wrap dominant hand onto lead bicep ($Right Grip > 0.65, handsDistSledge < 0.20\text{ m}$), and squeeze both triggers ($> 0.60$) for $> 300\text{ ms}$.
- **Synthesized Controls**: Emits `W` + `C` + `Space` + `LMB` for standing head-and-arm submission constriction and collapse.
- **Haptic Feedback**: Dual $140\text{ Hz}$ ($0.60$) vascular occlusion compression hum ramping into $280\text{ Hz}$ ($0.98$) cervical submission snap.

## Consequences
- Adds an authentic, high-leverage submission finish to Riddick's bareknuckle pit brawl repertoire.
- Synthesizes smoothly with existing clinch positions (guillotine choke, collar tie, kimura).
