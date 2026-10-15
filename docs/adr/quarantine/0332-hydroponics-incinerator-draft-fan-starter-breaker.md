# ADR 0332: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Circuit Breaker Interlock Close

## Status
Accepted

## Context
With the draft fan rotor dynamically balanced and vibration diagnostics zero-calibrated (ADR 0328), energizing the high-pressure induced draft fan motor requires manually throwing the 480V 3-phase motor starter circuit breaker lever into the closed position, engaging main bus power to the combustion air induction assembly.

## Decision
Implement physical draft fan motor starter circuit breaker throw mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the motor starter enclosure door flange (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand grips the heavy cast-iron breaker switch handle (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Downward Throw Stroke**: Forcefully throw dominant hand downward and forward (`rVel.y < -0.85\text{ m/s} \land rVel.z < -0.50\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` + `Enter` to trip the mechanical interlock bar and seat high-voltage copper bus contacts into closed position.
- **Haptic Feedback**: $370\text{ Hz}$ ($0.98$) spring-loaded bus contact closure snap on dominant controller and $135\text{ Hz}$ ($0.58$) electromagnetic inductive hum rumble on off-hand controller.

## Consequences
- Energizes 480V motor starter circuit for induced draft fan.
- Clears system for automated thermal combustion induction cycle in subterranean waste incinerator.
