# ADR 0267: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Heat Sink Clamp Spring

## Status
Accepted

## Context
During EVA spacewalk maintenance on Dark Athena solar arrays, solar flare radiation surges require functional bypass diodes across photovoltaic string junctions to prevent localized hotspot thermal destruction. Installing a beryllium copper spring clamp over the diode heat sink flange seats the thermal interface pad and prevents diode burnout under extreme stellar irradiation.

## Decision
Implement physical bypass diode heat sink clamp spring mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the diode heat sink housing (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns the spring clamp applicator (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Spring Compression Stroke**: Drive the right hand downward across the clamp flange (`rVel.y < -0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` to engage the spring clip, compress the thermal pad, seat the bypass diode, and confirm electrical continuity.
- **Haptic Feedback**: $280\text{ Hz}$ ($0.95$) tempered spring-steel snap on the dominant controller and $140\text{ Hz}$ ($0.55$) heat sink pad compression on the off-hand controller.

## Consequences
- Protects solar collector photovoltaic strings from thermal breakdown during high-energy solar storms.
- Complements laser weld pyrometer inspections, carabiners, and pouch sealing.
