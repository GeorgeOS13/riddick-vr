# ADR 0033: Vent Duct Pipe Hang and Aerial Drop Takedowns

## Status
Accepted

## Context
Riddick's lethal predatory identity centers around verticality and ambush:
1. **Vertical Ambush:** In both Butcher Bay and Dark Athena, Riddick crawls through overhead maintenance ducts and hangs from suspended steam pipes above patrol paths.
2. **Death From Above:** Dropping silently from overhead ducts directly onto an enemy below executes an instant stealth kill, crushing the victim beneath Riddick's boots and dual blades.
3. **VR Physicality:** In VR, looking down over the ledge of a vent duct or releasing one's grip from an overhead pipe creates an adrenaline-charged drop that demands tactile weapon impact upon touchdown.

## Decision

We implement **Aerial Drop Ambush & Pipe Hang Kinematics** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
             [Player Peering Down from High Ledge / Pipe Hang]
             [Pitch < -45 degrees (Looking Down)] OR [Hands Raised Y > 0.35m]
                                   │
                                   ▼
             [Drop / Downward Plunge Executed: V_y < -1.10 m/s]
                                   │
                                   ▼
             [Aerial Drop Assassination Triggered ('LMB')]
             - Executes lethal aerial takedown upon collision
             - Delivers dual-hand 0.95 bone-shattering ground impact haptics
```

### 1. Aerial Ledge / Overhead Pipe Hang Stance
- Detects when the player looks steeply downward ($\text{pitch} < -45^\circ$) or raises both hands overhead ($Y_{local} > +0.35\text{ m}$) while near a ledge.
- Arms the aerial drop assassination state.

### 2. Downward Plunge Impact
- When the player executes a downward plunge or punch ($V_y < -1.10\text{ m/s}$), the system triggers an immediate primary attack (`mouse_event(MOUSEEVENTF_LEFTDOWN, ...)`).
- Both controllers emit a crushing $0.95$ amplitude impact shockwave, simulating Riddick landing squarely on his quarry.

## Consequences

### Positive
- Delivers the signature Riddick predator fantasy from overhead perches.
- Seamlessly blends vertical level design with visceral 6DOF motion combat.
