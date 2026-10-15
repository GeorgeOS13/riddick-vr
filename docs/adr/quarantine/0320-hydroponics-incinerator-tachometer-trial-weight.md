# ADR 0320: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Correction Trial Weight Clip Installation

## Status
Accepted

## Context
With the polar coordinate graticule display locking the rotor's unbalance angle and amplitude, sliding and crimping a calibrated 25-gram steel dynamic balance trial weight clip onto the indicated radial blade rib compensates for centrifugal eccentricity, dampening violent casing vibration before high-velocity draft fan re-engagement.

## Decision
Implement physical draft fan dynamic balance trial weight clip installation mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand stabilizes the fan impeller shroud rim (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand aligns the calibrated spring-steel balance clip onto the designated radial blade rib (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Forward Push & Crimp Stroke**: Drive right hand forward into the blade plane (`rVel.z < -0.80\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` to slide trial clip onto blade rib and seat retention barb.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.95$) spring-steel balance clip snap seating thud on dominant controller and $140\text{ Hz}$ ($0.50$) impeller acoustic resonance on off-hand controller.

## Consequences
- Directly addresses rotor unbalance harmonics identified by the optical tachometer oscilloscope.
- Direct prerequisite to subsequent vibration verification and lock-tang staking.
