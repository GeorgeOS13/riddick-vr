# ADR 0336: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Variable Frequency Drive Speed Control Potentiometer Dial Adjustment

## Status
Accepted

## Context
With the motor starter circuit breaker energized (ADR 0332), ramping the induced draft fan up to its 3450 RPM operating point requires adjusting the 0–60Hz frequency command potentiometer on the VFD interface panel to achieve the target $-120\text{ Pa}$ negative furnace pressure for bio-waste incineration.

## Decision
Implement physical draft fan VFD speed control potentiometer adjustment mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand braces the VFD enclosure chassis frame (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand pinches the knurled multi-turn speed dial (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Rotational Ramp Stroke**: Rotate dominant hand in a clockwise dial-ramp motion (`rVel.x > 0.65\text{ m/s} \land rVel.y > 0.45\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` + `Enter` + `Q` to adjust inverter PWM output frequency and accelerate the draft fan impeller.
- **Haptic Feedback**: $360\text{ Hz}$ ($0.95$) precision multi-turn potentiometer drag on dominant controller and $140\text{ Hz}$ ($0.55$) high-frequency IGBT inverter switching hum on off-hand controller.

## Consequences
- Ramps induced draft fan motor speed to 3450 RPM.
- Stabilizes furnace negative pressure for automated bio-waste combustion.
