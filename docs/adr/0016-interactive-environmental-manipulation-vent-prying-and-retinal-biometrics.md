# ADR 0016: Interactive Environmental Manipulation, Ventilation Grate Prying, and Retinal Biometrics

## Status
Accepted

## Context
*The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena* feature prominent environmental physical problem solving:
1. **Ventilation Duct Traversal:** Prying open steel maintenance grates to bypass security checkpoints and patrol routes.
2. **Biometric Retinal Scanners & Security Consoles:** Activating iris recognition scanners and computer terminals to release door bulkheads.
3. **Container Search & Locker Access:** Interacting with weapon cabinets, medical nanotech stations, and supply lockers.

In the original flat game, all contextual interactions were unified into a single button press (`E`). In VR, pressing a face button while standing in front of a heavy steel vent cover breaks immersion. Players naturally expect to reach forward with both hands, grab the edges of the grate, wrench it backward, or extend an index finger to tap a biometric terminal.

## Decision

We implement a **Spatial Environmental Kinematic Engine** in `src/hook/camera_hook.cpp`:

```
                 [Player Controller Poses & Velocities]
                                   │
         ┌─────────────────────────┴─────────────────────────┐
         ▼                                                   ▼
[Vent Grate Prying: Dual-Hand Pull]        [Biometric Terminal / Retinal Poke]
- Both hands forward: Z < -0.20m           - Index pointing: trig < 0.20, grip > 0.40
- Both grips held (> 0.50f)                - Forward poke: Z < -0.34m, Vz < -0.28 m/s
- Pull back: Vz_left > 0.28 & Vz_right > 0.28 - Dispatches 'E'
- Dispatches 'E' + Space                   - Digital tactile confirmation (0.40)
- Dual metallic wrenching haptics (0.75)
```

### 1. Ventilation Grate Prying (Dual-Hand Pull)
- **Kinematic Attachment:**
  When both controllers are positioned in front of the chest/face ($Z_{local} < -0.20\text{ m}$) and both grip buttons are squeezed ($\text{grip} > 0.50$), the player grabs the virtual grate handles.
- **Wrenching Motion:**
  Pulling both hands backward towards the torso ($V_{z, left} > +0.28\text{ m/s} \land V_{z, right} > +0.28\text{ m/s}$) triggers the grate detachment sequence, dispatching `E` + `Space` and emitting high-energy dual metallic haptics ($0.75$).

### 2. Biometric Retinal & Terminal Touch (Pointed Poke)
- **Pointing Hand Posture:**
  Evaluates pointing state when the trigger is relaxed ($\text{trigger} < 0.20$) while the grip is held ($\text{grip} > 0.40$).
- **Terminal Plane Penetration:**
  When the pointing hand pushes forward into the interaction zone ($Z_{local} < -0.34\text{ m}$) with a positive poke velocity ($V_z < -0.28\text{ m/s}$), the system dispatches `E` with a crisp digital tactile beep ($0.40$).

### 3. State Isolation & Filtering
- **Debounce:** An $800\text{ ms}$ temporal gate prevents chatter or multiple activations during a single pull or poke stroke.
- **Conflict Suppression:** Interaction gestures are disabled while actively climbing ladders (`isClimbingActive`), holding a boxer guard (`s_physicalBlockEngaged`), or steadying a firearm (`s_twoHandedStabilizationActive`).

## Consequences

### Positive
- Fully physical, visceral interaction with Butcher Bay's vents, terminals, and security doors.
- Distinct tactile signatures distinguishing heavy steel grate prying from subtle digital biometric touches.
- Zero reliance on artificial floating crosshairs or immersion-breaking button prompts.
