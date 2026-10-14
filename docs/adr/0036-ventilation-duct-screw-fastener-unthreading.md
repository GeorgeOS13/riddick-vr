# ADR 0036: Ventilation Duct Screw Fastener Unthreading and Prying

## Status
Accepted

## Context
Across Butcher Bay (e.g. Prison Cell Block, Infirmary, and Sewer Intake):
1. **Fastened Grates:** Many ventilation grates are secured by heavy industrial corner screws requiring a screwdriver or shiv to bypass.
2. **Desktop Mechanics:** In the 2004 PC release, pressing 'E' while facing the grate played a brief scripted interaction.
3. **VR Physicality:** In VR, players expect to reach to the corners of the grate and physically twist their wrist in counter-clockwise revolutions to unthread fasteners, culminating in pulling the unfastened grate free.

## Decision

We implement **Rotational Wrist Twist Fastener Removal** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Tool / Shiv Contact at Vent Corner]
                   [Z_local in [-0.45, -0.20]m, Grip > 0.45]
                                    │
                                    ▼
           [Counter-Clockwise Wrist Twist: Angular Speed > 3.0 rad/s]
                                    │
                                    ▼
           [Fastener Unthreading Step Executed]
           - Injects tactile mechanical ratchet click: 0.45 Amplitude (160 Hz)
           - Advances grate unfastening sequence ('E' pulse)
```

### 1. Grate Corner Contact Detection
- Detects the dominant hand contacting the vent perimeter plane ($Z_{local} \in [-0.45, -0.20]\text{ m}, |X_{local}| > 0.12\text{ m}$).

### 2. Rotational Wrist Velocity Tracking
- Tracks roll velocity of the controller: when rotating counter-clockwise at $> 3.0\text{ rad/s}$, emits a tactile mechanical ratchet tooth click ($0.45$ amplitude at $160\text{ Hz}$) and unfastens the plate.

## Consequences

### Positive
- Turns vent infiltration into an engaging physical lockpicking/unfastening puzzle.
- Seamlessly integrates with the dual-hand vent prying mechanic (ADR 0016).
