# ADR 0180: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Purge Steam Lance

## Status
Accepted

## Context
Deep in the subterranean hydroponics and waste recycling facility beneath Butcher Bay / Dark Athena lower decks, heavy bio-waste buildup clogs the high-pressure incinerator exhaust manifolds. Riddick grabs the heavy high-pressure purge steam lance with both hands, seats the lance nozzle into the purge receiver port, and thrusts forward while holding the lance trigger to blast caked bio-tar with superheated steam.

## Decision
Implement physical two-handed purge steam lance operation in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position hands at the incinerator purge manifold receiver ($Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.25, 0.20]\text{ m}$) with two-handed grip stance ($dist < 0.28\text{ m}$).
- **Lance Seating & Purge Burst**: Both controller grips held ($Grip > 0.60$), dominant trigger held ($Trigger > 0.65$), delivering a forward linear thrust ($V_z < -0.90\text{ m/s}$) held for $> 300\text{ ms}$ to blast the purge steam lance into the manifold.
- **Synthesized Controls**: Emits `E` + `Q` + `LMB` + `Space` for valve actuation, tool engagement, and pneumatic steam injection.
- **Haptic Feedback**: Dual $110\text{ Hz}$ ($0.70$) pressurized steam hiss vibration ramping into $240\text{ Hz}$ ($0.95$) manifold purge acoustic blast.

## Consequences
- Deepens industrial maintenance puzzle mechanics in subterranean environments.
- Integrates seamlessly with existing bio-waste incinerator systems (spud bar, flame damper, quartz sight-glass, scrubber drain).
