# ADR 0368: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Motor Starter Phase Unbalance Relay Trip Sensitivity Trimmer Potentiometer Adjustment

## Status
Accepted

## Context
In the Subterranean Hydroponics Bio-Waste Incinerator motor starter mezzanine, following phase loss test button actuation (ADR 0364), the voltage/current unbalance percentage trip threshold must be dialed in using a micro-trimmer potentiometer. In high-harmonic variable-frequency drive environments, setting the threshold to precisely 8% prevents nuisance tripping during flue gas surge transients while ensuring instantaneous trip if severe rotor unbalance occurs.

## Decision
Implement physical two-handed spatial gesture detection for the phase unbalance sensitivity trimmer potentiometer adjustment in `src/hook/camera_hook.cpp`:
1. **Phase Monitor Casing Support**: Left hand braces phase monitor casing ($Z \in [-0.42, -0.16]\text{ m}, Y \in [0.05, 0.35]\text{ m}$, Left Grip $> 0.55$).
2. **Ceramic Screwdriver Grip**: Right hand holds precision ceramic adjustment screwdriver ($dist < 0.22\text{ m}$, Right Grip $> 0.55 \land Right Trigger < 0.40$).
3. **Rotational Trimmer Sweep**: Clockwise/counter-clockwise rotational sweep with forward engagement ($|rVel.x| > 0.55\text{ m/s} \land rVel.z < -0.65\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Q'` + `'Tab'` + `'Return'` + `'Space'` + `'Q'` + `'Return'` + `'Space'` (`0x45`, `0x51`, `0x09`, `0x0D`, `0x20`, `0x51`, `0x0D`, `0x20`).
5. **Haptic Feedback**: $430\text{ Hz}$ ($0.95$, $120\text{ ms}$) trimmer wiper detent micro-click on right controller, $165\text{ Hz}$ ($0.55$, $150\text{ ms}$) starter chassis acoustic resonance on left controller.

## Consequences
- Stabilizes high-pressure ventilation fans against electrical harmonic fluctuations.
- Deepens interactive industrial electrical maintenance fidelity.
