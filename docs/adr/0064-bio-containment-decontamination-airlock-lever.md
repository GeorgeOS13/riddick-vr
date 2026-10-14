# ADR 0064: Bio-Containment Cryo-Chamber Decontamination Lever

## Status
Accepted / Implemented

## Context
Entering medical bio-containment cells in Butcher Bay (Facility Double-Max Infirmary) and biological research modules on Dark Athena requires cycling atmospheric decontamination airlocks. Heavy industrial wall levers regulate the high-pressure spray of disinfectant vapor and hermetic pressure equalization.

In VR, reaching upward to grab a vertical wall lever and hauling downward with physical resistance grounds the player in the diegetic mechanical world of Starbreeze's sci-fi architecture.

## Decision
We implement bio-containment airlock lever cycling in Section 6 of `src/hook/camera_hook.cpp`:
1. **Lever Reach Zone**:
   - Controller positioned at the overhead lever fixture:
     $Z_{local} \in [-0.38, -0.15]\text{ m}$, $Y_{local} \in [0.10, 0.40]\text{ m}$.
   - Hand grip engaged ($> 0.50$).
2. **Kinematic Downward Pull**:
   - Hauling the lever downward with velocity $V_y < -0.60\text{ m/s}$ trips the pneumatic release contacts.
3. **Pneumatic Venting & Pressure Equalization**:
   - Dispatches airlock cycle command (`E` + `Space`), triggering hermetic door seals and disinfectant gas release.
   - Emits a sustained $140\text{ Hz}$ ($0.75$ amplitude) pneumatic whoosh and mechanical detent clack.

## Consequences
- Provides authentic industrial physical feedback for airlock transitions.
- High vertical zone ($Y > 0.10\text{ m}$) prevents interference with chest holsters or table interactions.
- Prevents re-triggering via an $800\text{ ms}$ debounce cooldown.
