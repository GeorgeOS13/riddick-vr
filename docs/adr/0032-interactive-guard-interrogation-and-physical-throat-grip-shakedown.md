# ADR 0032: Interactive Guard Interrogation and Physical Throat-Grip Shakedown

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay*, Riddick navigates prison yards, medical wards, and mine complexes by extorting corrupt guards, inmates, and snitches:
1. **Intimidation & Extortion:** Obtaining keycards, security codes, shivs, and cigarette stashes requires confronting NPCs and coercing them through intimidation dialogue.
2. **VR Disconnect:** On desktop monitors, interaction is a passive press of the 'E' key while facing the NPC. In VR, Riddick physically grabs snitches by the collar or throat, shaking them against walls or fences until they surrender the information.

## Decision

We implement **Physical Throat-Grip Shakedown Gestures** in Section 9 of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                 [Player Extends Left Hand Forward]
           [Z_local in [-0.60, -0.22]m, Y_local in [-0.25, +0.15]m]
                                 │
                                 ▼
                     [Left Grip Clenched > 0.50]
                     - Grabs Collar / Throat
                     - Emits 0.45 Throaty Struggle Pulse
                                 │
                                 ▼
                 [Physical Forward/Back Shake: |Vel_z| > 0.70 m/s]
                     - Shakes NPC Aggressively
                     - Injects Intimidation Tap ('E')
                     - Delivers 0.75 Compliance Snap Haptic
```

### 1. Reach & Collar Grab Kinematics
- Evaluates when the left hand is held forward at chest/collar height ($Z_{local} \in [-0.60, -0.22]\text{ m}, Y_{local} \in [-0.25, +0.15]\text{ m}$) with Left Grip squeezed.
- Emits a low-frequency $120\text{ Hz}$ throaty resistance pulse ($0.45$ amplitude).

### 2. Forward/Back Shake Dynamics
- When the player thrusts and pulls their clenched off-hand along the Z-axis ($|V_z| > 0.70\text{ m/s}$), the mod detects a violent shakedown gesture.
- Issues a dialogue advance pulse (`SendKeyTap('E')`) and delivers a sharp compliance tactile snap ($0.75$ amplitude at $220\text{ Hz}$).

## Consequences

### Positive
- Transforms static NPC conversations into raw, tactile Riddick-style shakedowns.
- Fully compatible with standard dialogue trees while adding visceral physical presence.
