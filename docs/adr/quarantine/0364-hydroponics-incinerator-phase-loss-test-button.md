# ADR 0364: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Phase Loss Sensor Test Button Actuation

## Status
Accepted

## Context
In the Subterranean Hydroponics Bio-Waste Incinerator motor starter mezzanine, following phase reversal latch verification (ADR 0360), the solid-state phase loss detection module must be test-tripped. A lost phase in high-current 480V motor circuits produces extreme unbalance current that can melt motor stator coils within seconds. Depressing the tactile test button simulates single-phasing to ensure instantaneous breaker disconnect.

## Decision
Implement physical two-handed spatial gesture detection for the phase loss sensor test button actuation in `src/hook/camera_hook.cpp`:
1. **Phase Monitor Chassis Support**: Left hand braces phase monitor mounting rail inside starter enclosure ($Z \in [-0.42, -0.16]\text{ m}, Y \in [0.05, 0.35]\text{ m}$, Left Grip $> 0.55$).
2. **Plunger Target Alignment**: Right index finger points at red recessed test button ($dist < 0.22\text{ m}$, Right Grip $< 0.45 \land Right Trigger < 0.35$).
3. **Linear Contact Plunge**: Rapid forward plunge stroke ($rVel.z < -0.75\text{ m/s}$) into test button seat.
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Q'` + `'Tab'` + `'Return'` + `'Space'` + `'Q'` + `'Return'` (`0x45`, `0x51`, `0x09`, `0x0D`, `0x20`, `0x51`, `0x0D`).
5. **Haptic Feedback**: $425\text{ Hz}$ ($0.95$, $120\text{ ms}$) micro-switch tactile snap on right controller, $160\text{ Hz}$ ($0.55$, $150\text{ ms}$) starter frame acoustic resonance on left controller.

## Consequences
- Prevents catastrophic stator burnout by certifying automatic single-phase cutouts.
- Extends the diegetic electrical system interaction depth.
