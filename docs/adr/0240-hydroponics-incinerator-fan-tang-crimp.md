# ADR 0240: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Lock Tang Crimping Plier Squeeze

## Status
Accepted

## Context
Centrifugal acceleration exceeding $3,000\text{ RPM}$ during peak bio-waste incineration can dislodge newly inserted balance weight clips. Securing the clip requires applying mechanical crimping pliers across the retention tang, deforming the spring-steel lip over the impeller blade stiffener rib.

## Decision
Implement physical balance clip tang crimping mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces against the fan blade root through the inspection port (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand applies crimping pliers over the balance clip tang (`twoHandDist < 0.22\text{ m}`, right grip $> 0.60$, right trigger $> 0.60$).
- **Linear Crimping Stroke**: Deliver a firm forward linear crimping squeeze with the right controller (`rVel.z < -0.80\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` to crush the locking tang, stake the clip, and lock the impeller assembly.
- **Haptic Feedback**: $310\text{ Hz}$ ($0.98$) plier jaw mechanical bottom-out snap on the dominant controller and $170\text{ Hz}$ ($0.65$) blade deformation resistance on the off-hand.

## Consequences
- Prevents balance clip shedding during thermal runaway and high-pressure induced draft surges.
- Complements clip insertion, bearing sight-glass venting, and cleanout dog-clamping.
