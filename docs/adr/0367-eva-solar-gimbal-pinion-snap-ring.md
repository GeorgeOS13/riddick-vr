# ADR 0367: Dark Athena EVA Solar Collector Photovoltaic Array Primary Sun-Tracking Gimbal Pitch Quadrant Drive Pinion Bearing Oil Seal Press Ring Snap Ring Seating

## Status
Accepted

## Context
On the exterior photovoltaic array gimbal of the Dark Athena, after locking the pinion castle nut with the cotter pin (ADR 0363), the fluorosilicone bearing oil seal press ring must be retained with an internal circlip / snap ring seated into the housing circumferential groove. This hermetically seals the synthetic fluorinated grease from venting into hard space vacuum ($10^{-9}\text{ torr}$) and stops particulate micrometeoroid ingestion under extreme solar thermal cycling.

## Decision
Implement physical two-handed spatial gesture detection for the pitch quadrant drive pinion bearing oil seal press ring snap ring seating in `src/hook/camera_hook.cpp`:
1. **Quadrant Bearing Cover Boss Support**: Left hand braces quadrant bearing front cover boss ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$).
2. **Snap Ring Pliers Grip**: Right hand holds snap ring pliers ($dist < 0.22\text{ m}$, Right Grip $> 0.55 \land Right Trigger > 0.50$).
3. **Plunge & Seat Stroke**: Forward compression plunge into snap ring groove and lateral wrist twist ($rVel.z < -0.75\text{ m/s} \land |rVel.x| > 0.50\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'E'` + `'Space'` + `'Tab'` + `'Return'` + `'Space'` + `'Q'` + `'Return'` + `'Space'` (`0x45`, `0x20`, `0x09`, `0x0D`, `0x20`, `0x51`, `0x0D`, `0x20`).
5. **Haptic Feedback**: $430\text{ Hz}$ ($0.95$, $120\text{ ms}$) circlip detent expansion snap on right controller, $175\text{ Hz}$ ($0.55$, $140\text{ ms}$) bearing cover acoustic vibration on left controller.

## Consequences
- Guarantees space vacuum seal integrity for exterior tracking gimbal gearboxes.
- Seamlessly integrates with the Section 6 environmental gesture dispatch pipeline.
