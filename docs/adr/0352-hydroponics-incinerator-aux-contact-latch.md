# ADR 0352: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Auxiliary Contact Block Mechanical Latch Verification

## Status
Accepted

## Context
In the Subterranean Hydroponics Bio-Waste Incinerator motor starter mezzanine, after closing the circuit breaker (ADR 0332) and visually verifying the green RUN pilot strobe lamp (ADR 0348), the operator must verify that the motor starter's auxiliary contact block has mechanically latched. This auxiliary contact block feeds the running status to the incinerator purge cycle logic and enables the downstream burner ignition interlock.

## Decision
Implement physical two-handed spatial gesture detection for the auxiliary contact block mechanical latch plunger actuation in `src/hook/camera_hook.cpp`:
1. **Mounting Frame Support**: Left hand braces auxiliary contact block mounting frame ($Z \in [-0.42, -0.16]\text{ m}, Y \in [0.05, 0.35]\text{ m}$, Left Grip $> 0.55$).
2. **Plunger Tip Alignment**: Right index finger points at yellow mechanical latch plunger ($dist < 0.22\text{ m}$, Right Grip $< 0.45 \land Right Trigger < 0.35$).
3. **Linear Contact Stroke**: Rapid forward linear drive into latch seat ($rVel.z < -0.75\text{ m/s}$) to verify positive mechanical contact engagement.
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Q'` + `'Tab'` + `'Return'` + `'Enter'` + `'Space'` (`0x45`, `0x51`, `0x09`, `0x0D`, `0x0D`, `0x20`).
5. **Haptic Feedback**: $410\text{ Hz}$ ($0.95$, $120\text{ ms}$) mechanical auxiliary contact latch snap on right controller, $150\text{ Hz}$ ($0.55$, $150\text{ ms}$) starter enclosure resonance on left controller.

## Consequences
- Completes the electrical verification chain for the bio-waste incinerator induced draft fan.
- Enhances tactile realism during facility reactivation sequences.
