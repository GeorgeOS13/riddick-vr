# ADR 0315: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Silicone Insulator Boot Nylon Cable Tie Tension & Flush Cutter Snip

## Status
Accepted

## Context
After sliding the silicone boot over the bypass diode terminal post and hermetically sealing the collar, securing the neck of the boot with a high-tensile radiation-hardened nylon cable tie and tensioning it with a pistol-grip tie tensioner gun before flush-cutting the excess tail prevents vacuum migration or thermal-cycling slippage.

## Decision
Implement physical bypass diode silicone boot cable tie tensioning and flush cutter snip mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand stabilizes the diode bracket collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns pistol-grip cable tie gun over tie tail (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Lateral Tension Stroke**: Draw dominant hand laterally across tie axis (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Return` to tension tie strap to 120 N and flush-cut excess tail cleanly.
- **Haptic Feedback**: $330\text{ Hz}$ ($0.95$) mechanical guillotine blade flush-cut snap on dominant controller and $140\text{ Hz}$ ($0.50$) heatsink bracket reaction on off-hand controller.

## Consequences
- Permanently locks silicone insulator boot to diode terminal junction.
- Direct prerequisite for array harness continuity validation and final bus energization.
