# ADR 0356: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Thermal Overload Relay Manual Reset Plunger Trip

## Status
Accepted

## Context
In the Subterranean Hydroponics Bio-Waste Incinerator motor starter mezzanine, prior to initiating continuous bio-waste combustion and high-pressure induced draft ventilation, the bimetallic thermal overload relay must be manually test-tripped and reset. This operational procedure verifies that the protective trip mechanism will release the starter coil contactors in the event of phase loss or impeller jam, preventing catastrophic motor winding failure.

## Decision
Implement physical two-handed spatial gesture detection for the thermal overload relay manual test-trip and reset plunger actuation in `src/hook/camera_hook.cpp`:
1. **Relay Rail Support**: Left hand braces lower relay mounting rail inside starter enclosure ($Z \in [-0.42, -0.16]\text{ m}, Y \in [0.05, 0.35]\text{ m}$, Left Grip $> 0.55$).
2. **Plunger Target Alignment**: Right index finger points at blue test/reset plunger ($dist < 0.22\text{ m}$, Right Grip $< 0.45 \land Right Trigger < 0.35$).
3. **Linear Contact Plunge**: Rapid forward plunge stroke ($rVel.z < -0.75\text{ m/s}$) into overload reset seat.
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Q'` + `'Tab'` + `'Return'` + `'Space'` + `'Q'` (`0x45`, `0x51`, `0x09`, `0x0D`, `0x20`, `0x51`).
5. **Haptic Feedback**: $410\text{ Hz}$ ($0.95$, $120\text{ ms}$) bimetallic spring snap on right controller, $150\text{ Hz}$ ($0.55$, $150\text{ ms}$) starter frame acoustic vibration on left controller.

## Consequences
- Completes safety certification for high-pressure draft fan systems.
- Extends diegetic electrical interactions with Starbreeze industrial machinery.
