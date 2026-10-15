# ADR 0355: Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Elevation/Pitch Quadrant Worm Gear Backlash Vernier Squeeze

## Status
Accepted

## Context
On the exterior photovoltaic array gimbal of the Dark Athena, the primary elevation/pitch drive couples through a curved quadrant sector gear driven by a hardened worm shaft. After engaging the elevation limit travel stops (ADR 0351), mechanical backlash between the quadrant teeth and the worm thread must be precisely dialed out using an eccentric vernier pinch clamp. This prevents celestial tracking jitter and structural shudder caused by main maneuvering thruster firings.

## Decision
Implement physical two-handed spatial gesture detection for the elevation quadrant worm gear backlash vernier adjustment in `src/hook/camera_hook.cpp`:
1. **Quadrant Sector Flange Support**: Left hand braces elevation quadrant sector mounting flange ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$).
2. **Vernier Clamp Pinch**: Right hand pinches eccentric backlash vernier adjustment collar ($dist < 0.22\text{ m}$, Right Grip $> 0.55 \land Right Trigger > 0.50$).
3. **Eccentric Squeeze Stroke**: Squeeze and rotate clockwise around vernier collar ($rVel.z < -0.75\text{ m/s} \land |rVel.x| > 0.50\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Space'` + `'Tab'` + `'Return'` + `'Space'` + `'Q'` (`0x45`, `0x20`, `0x09`, `0x0D`, `0x20`, `0x51`).
5. **Haptic Feedback**: $410\text{ Hz}$ ($0.95$, $120\text{ ms}$) eccentric backlash detent click on right controller, $160\text{ Hz}$ ($0.55$, $140\text{ ms}$) sector gear mesh resonance on left controller.

## Consequences
- Ensures smooth, vibration-free tracking behavior for exterior solar array gimbals.
- Integrates cleanly with the Section 5 gesture dispatch pipeline.
