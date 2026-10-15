# ADR 0348: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Run Pilot Indicator Strobe Lamp Visual Verification

## Status
Accepted

## Context
In the Subterranean Hydroponics Bio-Waste Incinerator control mezzanine, once the high-pressure induced draft fan variable frequency drive (VFD) starter circuit breaker is closed (ADR 0332), operating speed potentiometer adjusted (ADR 0336), acceleration ramp configured (ADR 0340), and motor RUN pushbutton latched (ADR 0344), the industrial high-intensity green RUN pilot indicator strobe lamp illuminates. The operator must visually inspect the lamp and depress the push-to-test bezel contact to verify filament and strobe circuit integrity, clearing the startup interlock latch.

## Decision
Implement physical two-handed spatial gesture detection for the draft fan motor run pilot indicator strobe lamp test actuation in `src/hook/camera_hook.cpp`:
1. **Control Panel Top Bezel Brace**: Left hand braces upper bezel rim of VFD control enclosure ($Z \in [-0.42, -0.16]\text{ m}, Y \in [0.05, 0.35]\text{ m}$, Left Grip $> 0.55$).
2. **Push-to-Test Bezel Aim**: Right index finger extends toward strobe indicator lens assembly ($dist < 0.22\text{ m}$, Right Grip $< 0.45 \land Right Trigger < 0.35$).
3. **Linear Contact Stroke**: Rapid forward poke stroke into lamp housing ($rVel.z < -0.75\text{ m/s}$) to depress spring-loaded push-to-test contact.
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Q'` + `'Tab'` + `'Return'` + `'Space'` (`0x45`, `0x51`, `0x09`, `0x0D`, `0x20`).
5. **Haptic Feedback**: $400\text{ Hz}$ ($0.95$, $120\text{ ms}$) lens cap micro-switch spring click on right controller, $145\text{ Hz}$ ($0.55$, $150\text{ ms}$) VFD enclosure acoustic vibration on left controller.

## Consequences
- Completes the operational startup sequence for the subterranean bio-waste draft fan system.
- Reinforces high-fidelity diegetic interaction with Starbreeze industrial equipment.
