# ADR 0029: Diegetic Collectible Inspection: Cigarette Packs and Bounty Cards

## Status
Accepted

## Context
Across Butcher Bay and Dark Athena, hidden collectibles reward exploration:
1. **Butcher Bay Cigarette Packs:** 59 packs concealed in dark corners, vent shafts, and inmate cells, unlocking concept art, audio commentary, and background lore.
2. **Dark Athena Bounty Cards:** Tech cards detailing notorious mercenary profiles.
3. **VR Disconnect:** On desktop monitors, picking up a pack triggers a brief 2D text notification. In Virtual Reality, players naturally lift the item to eye level to turn it over in their hands, examine the vintage sci-fi branding, and read the pack markings.

## Decision

We implement **Spatial Eye-Level Inspection** in Section 9 of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Player Holding Carried Object / Pack]
                                     │
                                     ▼
             [Brought up to Eye Level: Y > -0.05m & Z > -0.32m]
                                     │
                                     ▼
         [Diegetic Collectible Inspection Mode Engaged]
         - Emits subtle tactile inspection buzz: 0.25 Amplitude (180 Hz)
         - 6DOF hand rotation directly inspects item surfaces and art
```

### 1. Eye-Level Hand Elevation Kinematics
- Evaluates when `s_leftGripDown` is active and the off-hand controller is raised near eye level ($Y_{local} > -0.05\text{ m} \land Z_{local} > -0.32\text{ m}$).

### 2. Micro-Haptic Inspection Feedback
- Delivers a delicate $0.25$ amplitude pulse at $180\text{ Hz}$, tactilely confirming entry into close-up inspection distance.

## Consequences

### Positive
- Encourages tactile, immersive exploration for all 59 cigarette packs and bounty cards.
- Preserves full physical presence during narrative discovery.
