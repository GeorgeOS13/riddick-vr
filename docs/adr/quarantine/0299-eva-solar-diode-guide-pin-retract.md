# ADR 0299: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Ceramic Wafer Alignment Guide Pin Retraction & Latch Engagement

## Status
Accepted

## Context
Following the precision concentric seating of the bypass diode ceramic wafer and initial terminal torquing, withdrawing the alignment guide pin allows the spring-loaded phosphor-bronze retention latch to snap over the wafer edge, securely clamping the diode against the heatsink under high-g orbital maneuvers.

## Decision
Implement physical bypass diode guide pin retraction and latch engagement mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand steadies the heatsink diode pocket collar (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand pinches the guide pin knurled extraction head (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Linear Rearward Extraction Stroke**: Retract the right hand rearward along the bore axis (`rVel.z > 0.75\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` to withdraw the pin, release spring latch, seat latch over ceramic bevel, and lock diode wafer.
- **Haptic Feedback**: $280\text{ Hz}$ ($0.95$) spring latch snap on dominant controller and $140\text{ Hz}$ ($0.50$) heatsink pocket resonance on off-hand controller.

## Consequences
- Locks diode wafer mechanically in place before final silicone boot installation.
- Pairs directly with guide pin insertion, terminal screw torquing, and boot zip-tie cinching.
