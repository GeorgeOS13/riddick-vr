# ADR 0049: Heavy Mech Cockpit Canopy and Seal Purge Gestures

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* (Mech Core) and *Assault on Dark Athena* (Cargo Decks):
1. **Piloting Armored Combat Mechs:** Riddick hijacks heavy bipedal military mechs equipped with twin chainguns and heavy armor plates.
2. **Cockpit Ingress & Canopy Latch:** Entering a mech requires climbing into the armored cradle and pulling the reinforced overhead canopy lid down until pneumatic pressure locks seal the pilot capsule.
3. **Emergency Seal Purge:** Under catastrophic damage, the pilot must purge the canopy seal bolts to escape before reactor blowout.

## Decision

We implement **Overhead Canopy Slam & Dual-Stick Cockpit Purge** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Pilot Enters Armored Mech Cradle]
                   [Both Hands Overhead: Y_local > +0.20m, Grips > 0.50]
                                    │
                                    ▼
           [Downward Canopy Slam: Vy_left & Vy_right < -0.45 m/s]
                                    │
                                    ▼
           [Cockpit Sealed / Systems Online]
           - Injects canopy seal confirm ('E')
           - Delivers 0.85 pneumatic hermetic clamp surge (120 Hz)
                                    │
                                    ▼
           [Emergency Cockpit Purge / Ejection]
           - Dual Grip Clench + Both Thumbsticks Clicked
           - Blows canopy seal bolts with 0.95 explosive decompression wave
```

### 1. Overhead Canopy Slam Kinematics
- Measures both hands raised above shoulder height ($Y_{local} > +0.20\text{ m}$) with grips clenched, followed by a simultaneous downward slam ($V_y < -0.45\text{ m/s}$).
- Triggers pneumatic lock actuation with an $0.85$ amplitude pressure surge.

### 2. Dual-Grip Emergency Seal Purge
- Squeezing both grips ($> 0.60$) while clicking both thumbsticks executes an explosive cockpit canopy ejection, releasing Riddick safely onto his feet.

## Consequences

### Positive
- Massive cinematic immersion transitioning from foot stealth to colossal mech warfare.
- Emergency ejection offers a heroic escape from flaming mech wrecks.
