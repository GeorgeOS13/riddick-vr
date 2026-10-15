# ADR 0360: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Phase Reversal Protective Relay Latch Reset

## Status
Accepted

## Context
In the Subterranean Hydroponics Bio-Waste Incinerator motor starter mezzanine, following thermal overload verification (ADR 0356), the solid-state phase reversal protective relay latch must be test-tripped and reset. This relay prevents reverse impeller rotation if three-phase AC feeder lines suffer voltage phase inversion, which would otherwise pressurize explosive bio-waste methane back into the cultivation chambers rather than venting through the exhaust stack.

## Decision
Implement physical two-handed spatial gesture detection for the phase reversal protective relay latch reset in `src/hook/camera_hook.cpp`:
1. **Relay Chassis Support**: Left hand braces phase reversal relay mounting rail inside starter cabinet ($Z \in [-0.42, -0.16]\text{ m}, Y \in [0.05, 0.35]\text{ m}$, Left Grip $> 0.55$).
2. **Plunger Target Alignment**: Right index finger points at yellow phase reversal test/reset rocker plunger ($dist < 0.22\text{ m}$, Right Grip $< 0.45 \land Right Trigger < 0.35$).
3. **Linear Contact Plunge**: Rapid forward plunge stroke ($rVel.z < -0.75\text{ m/s}$) into phase reversal reset seat.
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Q'` + `'Tab'` + `'Return'` + `'Space'` + `'Q'` + `'Space'` (`0x45`, `0x51`, `0x09`, `0x0D`, `0x20`, `0x51`, `0x20`).
5. **Haptic Feedback**: $420\text{ Hz}$ ($0.95$, $120\text{ ms}$) solid-state protective relay latch snap on right controller, $155\text{ Hz}$ ($0.55$, $150\text{ ms}$) starter enclosure acoustic resonance on left controller.

## Consequences
- Guarantees directional airflow safety in subterranean bio-methane processing zones.
- Enriches industrial electrical interaction fidelity in VR.
