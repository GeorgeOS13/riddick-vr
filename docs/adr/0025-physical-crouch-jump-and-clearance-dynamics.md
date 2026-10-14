# ADR 0025: Physical Crouch-Jump, Knee-Tuck Clearance, and Infiltration Dynamics

## Status
Accepted

## Context
In *The Chronicles of Riddick*, level traversal and stealth infiltration require entering elevated air ducts, hopping over high security barriers, and navigating narrow pipe systems:
1. **Engine Jump Mechanics:** In Starbreeze's engine (as in id Tech and Source engines), jumping while standing clears standard obstacles. Clearing higher ledges or entering raised ventilation covers requires a "crouch-jump" (raising the bottom of the player's bounding hull by pressing `Crouch` while airborne).
2. **VR Disconnect:** In VR, a player who has physically ducked or knelt behind cover wants to spring forward onto a ledge or hop up into an air duct without having to fiddle with separate crouch toggle keys while pressing the jump button.

## Decision

We implement **Coordinated Crouch-Jump Clearance** in Section 10 of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Button B (Jump) Pressed]
                               │
               ┌───────────────┴───────────────┐
               ▼                               ▼
    [!s_isPhysicallyCrouched]       [s_isPhysicallyCrouched]
     (Standard Jump)                 (Physical Crouch-Jump Booster)
               │                               │
               ▼                               ▼
    - Dispatch VK_SPACE             - Dispatch 'C' (Hold) + VK_SPACE (Hold)
    - Standard Jump Clearance       - Lifts player collision hull +0.40m
    - 0.30 Haptic Impulse           - High Ledge / Air Duct Clearance
                                    - Solid 0.45 Haptic Launch Impulse
```

### 1. Dual-State Clearance Dispatch
- When `btnB` is pressed while the player is physically crouched (`s_isPhysicallyCrouched` is active):
  The hook synthesizes a simultaneous `C` (Crouch) and `VK_SPACE` (Jump) input packet.
- This immediately elevates Riddick's knee-tucked collision hull by $+0.40\text{ m}$, guaranteeing instant mantle clearance into raised ventilation ducts and high security catwalks.

### 2. Physical Launch Feedback
- Dispatches a $0.45$ launch impulse on the jumping hand, confirming the knee-tucked athletic clearance.

## Consequences

### Positive
- Fluid parkour and vertical duct traversal matching the player's physical body posture.
- Eliminates frustrating geometry collision snags when leaping into air vents.
