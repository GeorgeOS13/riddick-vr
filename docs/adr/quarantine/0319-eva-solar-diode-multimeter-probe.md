# ADR 0319: Dark Athena EVA Solar Collector Photovoltaic Array Bypass Diode Circuit Continuity & Resistance Multimeter Probe Test

## Status
Accepted

## Context
Following the mechanical installation and hermetic silicone boot sealing of the bypass diode, verifying electrical continuity, forward voltage drop ($0.58\text{ V}$), and circuit low-resistance junction integrity using two handheld digital multimeter test probes validates that current can bypass shaded or damaged photovoltaic wafer strings without localized thermal runaway.

## Decision
Implement physical bypass diode circuit continuity multimeter probe test mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds black ground multimeter probe against heatsink bracket ground lug (`leftLocal.z` between $-0.42\text{ m}$ and $-0.15\text{ m}$, `leftLocal.y` between $0.10\text{ m}$ and $0.40\text{ m}$, left grip $> 0.55$). Right hand aligns red positive probe tip against diode cathode test terminal (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Linear Forward Contact Stroke**: Drive right probe tip firmly into the terminal test receptacle (`rVel.z < -0.75\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` to measure forward junction voltage drop and verify circuit low-resistance continuity.
- **Haptic Feedback**: $310\text{ Hz}$ ($0.95$) sharp gold-plated probe tip contact snap on dominant controller and $150\text{ Hz}$ ($0.50$) ground clip reaction on off-hand controller.

## Consequences
- Electrically certifies diode polarity and low-resistance forward conduction before high-current bus energization.
- Crucial prerequisite to final solar collector articulation gimbal calibration.
