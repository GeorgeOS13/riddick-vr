# ADR 0021: Spatial Tactical Grenade Priming, Pin-Pulling, and Kinetic Trajectory Launch

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*, tactical concussion and frag grenades provide crucial crowd control against armored mercenaries and automated sentry drones.
In traditional flat gameplay:
1. Equipping grenades required cycling weapons or pressing `4`.
2. Throwing required holding `Left Mouse Button` to cook/aim and releasing to throw, or right-clicking to drop.

In Virtual Reality:
- Reaching down to click keyboard keys or scrolling a wheel breaks presence.
- Players expect a dedicated tactical chest bandolier where grenades are holstered.
- Furthermore, modern VR military and tactical standards (e.g. *Half-Life: Alyx*, *Boneworks*, *Pavlov*) establish a two-handed interaction pattern: the dominant hand holds the grenade while the off-hand reaches to the grenade spoon/pin, pulls it out, and the player physically hurls the munition.

## Decision

We implement **Spatial Bandolier Inventory & Kinetic Pin-Pulling** in Section 6c and 6e of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Right Controller at Chest Bandolier]
                   (Y in [-0.22, 0.05], |X| < 0.14, Z > -0.25)
                                   │
                                   ▼
             [Right Grip Squeezed -> Equips Slot '4' Grenades]
             - Mechanical Bandolier Snap Haptic (0.65)
                                   │
                                   ▼
          [Off-Hand (Left Controller) Brought to Grenade (< 16cm)]
                                   │
                                   ▼
             [Left Grip Squeezed -> Pulls Grenade Cotter Pin]
             - Sharp Metallic Pin Friction Haptic (0.75, 180 Hz)
             - Dispatches Mouse Left Down (Begins engine throw cook)
                                   │
                                   ▼
         [Physical Arm Swing (Speed > 1.2 m/s) OR Release Grip]
             - Dispatches Mouse Left Up
             - Propels Grenade along physical throwing vector
             - Kinetic Release Impulse (0.60)
```

### 1. Chest Rig Bandolier Holster
- Reaching towards the sternum / upper torso ($Y_{local} \in [-0.22, +0.05]\text{ m}, |X_{local}| < 0.14\text{ m}, Z_{local} > -0.25\text{ m}$) with right grip squeezed ($> 0.50$) equips weapon slot `4` (`SendKeyTap('4')`).
- Produces a distinct $0.65$ tactile snap.

### 2. Two-Handed Pin-Pull Kinematics
- Bringing the off-hand within $16\text{ cm}$ of the grenade and squeezing Left Grip pulls the arming pin.
- Hook executes `mouse_event(MOUSEEVENTF_LEFTDOWN)`, engaging the Starbreeze grenade throw cook mechanism.
- Emits a high-frequency $0.75$ metallic friction pulse ($180\text{ Hz}$).

### 3. Kinetic Release
- Swinging the arm ($V_{swing} > 1.2\text{ m/s}$) or releasing grip drops/throws the live grenade with `mouse_event(MOUSEEVENTF_LEFTUP)`, delivering a tactile release impulse ($0.60$).

## Consequences

### Positive
- Fully physical, two-handed grenade mechanics fulfilling modern VR expectations.
- Clean integration with existing holster slots (shoulder long-arms, hip sidearms, hip melee blades).
