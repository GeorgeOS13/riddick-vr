# ADR 0359: Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Pitch Quadrant Drive Pinion Tooth Grease Port Needle Lubrication

## Status
Accepted

## Context
On the exterior photovoltaic array gimbal of the Dark Athena, after adjusting the elevation quadrant worm gear backlash vernier (ADR 0355), the high-contact drive pinion teeth and worm thread require high-vacuum grease replenishment through an acoustic needle lubrication port. Applying precision extrusion into the pinion lube nipple dampens tooth micro-pitting under extreme orbital thermal cycling (+180°C solar exposure to -140°C planetary shadow eclipse).

## Decision
Implement physical two-handed spatial gesture detection for the pitch quadrant drive pinion tooth grease port needle lubrication in `src/hook/camera_hook.cpp`:
1. **Quadrant Drive Pinion Boss Support**: Left hand braces quadrant drive pinion housing / grease boss collar ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$).
2. **Grease Needle Dispenser Grip**: Right hand holds grease needle dispenser tool ($dist < 0.22\text{ m}$, Right Grip $> 0.55 \land Right Trigger > 0.50$).
3. **Needle Extrusion Stroke**: Forward needle insertion and vertical extrusion sweep ($rVel.z < -0.75\text{ m/s} \land |rVel.y| > 0.50\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Space'` + `'Tab'` + `'Return'` + `'Space'` + `'Q'` + `'Space'` (`0x45`, `0x20`, `0x09`, `0x0D`, `0x20`, `0x51`, `0x20`).
5. **Haptic Feedback**: $420\text{ Hz}$ ($0.95$, $120\text{ ms}$) grease port needle seating click on right controller, $165\text{ Hz}$ ($0.55$, $140\text{ ms}$) quadrant gear casing acoustic resonance on left controller.

## Consequences
- Protects EVA solar array mechanical drivetrains from catastrophic vacuum galling.
- Maintains single-level nesting and clean dispatch in Section 5/6 gestures.
