# ADR 0030: Laser Tripwire Dynamic Crouch-Limbo Evasion and Deep Prone Crawling

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* (notably in the High Security Facility and Tower 17 escape routes):
1. **Infrared / Laser Tripwires:** Red laser beams sweep across hallways and doorway thresholds. Tripping a beam triggers alarms and activates ceiling-mounted chaingun turrets.
2. **Physical Limbo / Prone Crawling:** Passing safely beneath low beams requires ducking much lower than an ordinary crouch. In VR, players instinctively get onto hands and knees or limbo under beams.

## Decision

We implement **Deep Prone / Limbo Detection and Crawl Pacing** in Section 1 and Section 3 of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Physical Head Height Drop > 48cm]
                                   │
                                   ▼
             [Stealth Stance: DEEP PRONE / LIMBO ENGAGED]
             - Tactile Engagement Pulse: 0.25 (85 Hz)
             - Signals safe passage beneath low laser tripwire planes
                                   │
                                   ▼
                   [Locomotion during Deep Prone]
             - Periodic stealth crawl pulse every 600ms (0.15)
             - Paces movement for silent, deliberate crawling
```

### 1. Multi-Tiered Crouch vs Prone Classification
- **Standard Crouch ($28\text{ cm} - 48\text{ cm}$ drop):** General stealth and cover-peeking.
- **Deep Prone / Limbo ($> 48\text{ cm}$ drop or $< 65\%$ standing height):** Extreme low profile for crawling under laser tripwires and low industrial vents.

### 2. Crawl Pacing Tactile Rhythm
- Injects a subtle $0.15$ amplitude pulse every $600\text{ ms}$ into the off-hand controller while moving in prone, conveying the deliberate cadence of crawling on hands and knees.

## Consequences

### Positive
- Thrilling stealth gameplay physically slithering beneath deadly laser security grids.
- Eliminates accidental turret alarms during careful infiltration.
