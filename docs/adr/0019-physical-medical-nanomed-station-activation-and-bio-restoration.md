# ADR 0019: Physical Medical NanoMED Health Station Activation and Bio-Restoration

## Status
Accepted

## Context
Across Butcher Bay and Dark Athena, player survivability depends on wall-mounted **NanoMED Medical Stations**:
1. **Health Regeneration:** White automated dispenser units that replenish Riddick's 4 health squares.
2. **NanoMED Cartridge Insertion:** Slotting medical canisters to reactivate depleted wall units.
3. **Diegetic Feedback:** Riddick resting his hand on the station as nanobots surge into his bloodstream.

In the original desktop game, walking up to a NanoMED station and pressing `E` triggered the healing sequence. In VR, pressing a face button on a controller feels disconnected. Players naturally walk up to the medical unit, open their palm, and press their hand against the dispenser pad.

## Decision

We implement **Flat Palm Environmental Detection** in Section 6d of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                     [Player Hand Approaching Surface]
                                     │
                    ┌────────────────┴────────────────┐
                    ▼                                 ▼
         [Pointed Finger: Poke]              [Open Palm: Touch]
         - Trigger < 0.20 & Grip > 0.40      - Trigger < 0.20 & Grip < 0.25
         - Forward Poke: Vz < -0.28 m/s      - Forward Press: Vz < -0.25 m/s
         - Z < -0.34m                        - Z < -0.30m
         - Dispatch 'E'                      - Dispatch 'E'
         - Sharp Digital Beep (0.40)         - Soothing Bio-Wave (0.50)
```

### 1. Palm Contact Kinematics
- **Relaxed Hand Pose:** Evaluates open palm state when both trigger and grip buttons are unpressed ($\text{trigger} < 0.20 \land \text{grip} < 0.25$).
- **Station Contact Plane:** When the open hand presses into the wall station plane ($Z_{local} < -0.30\text{ m}$) with forward velocity ($V_z < -0.25\text{ m/s}$), the system dispatches `SendKeyTap('E')`.

### 2. Soothing Bio-Restorative Tactile Pulse
- Delivers a $0.50$ amplitude pulse at $120\text{ Hz}$ ($75\text{ ms}$) on the interacting hand, conveying the biological nanite surge during healing.

## Consequences

### Positive
- Visceral tactile interaction with medical infrastructure.
- Clear distinction between sharp single-finger terminal pokes (retinal scanner / security consoles) and flat open palm contacts (NanoMED units).
