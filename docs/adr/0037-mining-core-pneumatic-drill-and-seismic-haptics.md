# ADR 0037: Mining Core Pneumatic Drill and Seismic Haptics

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* (Mining Core & Lower Mines):
1. **Pneumatic Rock Drills & Power Tools:** Riddick pilots industrial excavators and operates pneumatic jackhammers to shatter subterranean rock faces and blast walls.
2. **VR Disconnect:** On desktop monitors, heavy power equipment lacks physical weight. In VR, operating heavy equipment requires two-handed brace grips and violent, low-frequency motor rumble.

## Decision

We implement **Industrial Pneumatic Drill Vibration Synthesis** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Two-Handed Firearm / Tool Stance]
              [Both Triggers Fully Depressed > 0.80]
                                │
                                ▼
             [Seismic Pneumatic Hammering Active]
             - Injects cyclic 70ms high-impact pulses: 0.95 Amplitude (65 Hz)
             - Rumbles both controllers with subterranean seismic power
```

### 1. Dual-Trigger Industrial Engagement
- Detects two-handed tool grip when both the dominant and off-hand triggers are held down ($> 0.80$).

### 2. Low-Frequency Seismic Hammering
- Fires rapid $70\text{ ms}$ cyclic hammer pulses at $65\text{ Hz}$ with $0.95$ amplitude across both hands, accurately reproducing pneumatic percussion through rock.

## Consequences

### Positive
- Massive tactile physical presence when operating heavy industrial mining tools.
- Adds palpable visceral weight to Butcher Bay's subterranean mining chapters.
