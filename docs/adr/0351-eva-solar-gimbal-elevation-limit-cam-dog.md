# ADR 0351: Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Elevation/Pitch Limit Switch Cam Dog Engagement

## Status
Accepted

## Context
Following the engagement of the azimuth limit switch cam dog (ADR 0347) on the Dark Athena exterior photovoltaic array gimbal, the elevation/pitch axis limit switch cam dog must be set and engaged along its curved quadrant rail. This mechanical stop prevents the solar panel structure from exceeding maximum zenith elevation angles and colliding with the main structural truss members during automatic celestial tracking.

## Decision
Implement physical two-handed spatial gesture detection for the elevation limit switch adjustable cam dog engagement in `src/hook/camera_hook.cpp`:
1. **Housing Bracket Stabilization**: Left hand braces elevation limit switch housing on the clevis frame ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$).
2. **Cam Dog Slide Alignment**: Right hand aligns adjustable alloy cam dog slide shoe ($dist < 0.22\text{ m}$, Right Grip $> 0.55 \land Right Trigger < 0.40$).
3. **Quadrant Detent Engagement Stroke**: Linear sweep along arc rail into detent stop seat ($|rVel.y| > 0.65\text{ m/s} \land rVel.z < -0.50\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Space'` + `'Tab'` + `'Return'` + `'Enter'` (`0x45`, `0x20`, `0x09`, `0x0D`, `0x0D`).
5. **Haptic Feedback**: $410\text{ Hz}$ ($0.95$, $120\text{ ms}$) hardened alloy cam dog detent snap on right controller, $165\text{ Hz}$ ($0.55$, $140\text{ ms}$) limit switch housing resonance on left controller.

## Consequences
- Protects the physical solar array mechanism from catastrophic over-elevation during EVA operations.
- Fits seamlessly into the established environmental gesture pipeline.
