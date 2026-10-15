# ADR 0192: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Slag Tap Hole Oxygen Lance Burn

## Status
Accepted

## Context
When the bottom slag tap hole of the bio-waste incinerator solidifies into dense basaltic glass, standard spud bars cannot breach it. Riddick equips an ignited thermal oxygen lance with both hands, seats the burning tip into the tap hole, and pushes forward steadily while holding the oxygen valve trigger to melt through the slag crust.

## Decision
Implement physical two-handed thermal oxygen lance feed in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: In two-handed stance at the lower slag manifold ($dist < 0.26\text{ m}, Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.45, -0.10]\text{ m}$) with both grips clenched ($> 0.60$) and dominant trigger held ($> 0.70$).
- **Sustained Thermal Lance Feed**: Hold steady forward feed ($V_z < -0.30\text{ m/s}$) for $> 400\text{ ms}$ to pierce the solidified slag plug.
- **Synthesized Controls**: Emits `E` + `LMB` + `Space` + `Enter` for ignition arc, high-temperature thermal feed, and slag drain breach.
- **Haptic Feedback**: Sustained $260\text{ Hz}$ ($0.75$) exothermic chemical reaction sizzle ramping into $70\text{ Hz}$ ($0.98$) molten slag breakout rumble.

## Consequences
- Adds an authentic, intense thermal industrial maintenance mechanic to subterranean exploration.
- Works in concert with slag spud bars, incinerator blast doors, and flame dampers.
