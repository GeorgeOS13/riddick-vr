# ADR 0363: Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Pitch Quadrant Drive Pinion Bearing Retainer Castle Nut Cotter Pin Insertion & Tine Spread

## Status
Accepted

## Context
On the exterior photovoltaic array gimbal of the Dark Athena, after lubricating the pitch quadrant drive pinion teeth through the grease port (ADR 0359), the primary bearing retainer castle nut must be locked with a tempered stainless-steel cotter pin to prevent axial shaft displacement under intense orbital maneuvering dynamics. Riddick inserts the cotter pin through the drilled pinion shaft aperture and spreads the retention tines using needle-nose pliers.

## Decision
Implement physical two-handed spatial gesture detection for the pitch quadrant drive pinion bearing retainer castle nut cotter pin insertion and tine spread in `src/hook/camera_hook.cpp`:
1. **Quadrant Bearing Retainer Flange Support**: Left hand braces quadrant bearing retainer flange ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$).
2. **Cotter Pin Pliers Grip**: Right hand holds cotter pin pliers ($dist < 0.22\text{ m}$, Right Grip $> 0.55 \land Right Trigger > 0.50$).
3. **Pin Insertion & Spread Stroke**: Forward insertion stroke and horizontal tine spread sweep ($rVel.z < -0.75\text{ m/s} \land |rVel.x| > 0.50\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Space'` + `'Tab'` + `'Return'` + `'Space'` + `'Q'` + `'Return'` (`0x45`, `0x20`, `0x09`, `0x0D`, `0x20`, `0x51`, `0x0D`).
5. **Haptic Feedback**: $425\text{ Hz}$ ($0.95$, $120\text{ ms}$) cotter pin tine spread snap on right controller, $170\text{ Hz}$ ($0.55$, $140\text{ ms}$) bearing retainer housing resonance on left controller.

## Consequences
- Secures the pitch quadrant drive pinion against axial walk-out under heavy orbital thruster vibrations.
- Seamlessly integrates with the Section 6 environmental gesture dispatch pipeline.
