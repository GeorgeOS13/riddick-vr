# ADR 0179: Dark Athena EVA Solar Array Wing Jettison Explosive Bolt Shear

## Status
Accepted

## Context
On the exterior hull of the Dark Athena, catastrophic structural damage or solar storm flare overload requires jettisoning compromised solar array wings to prevent hull breach or electrical feedback. Riddick must access the wing jettison junction box, unseal the primary explosive bolt arming handle with both hands, and deliver a forceful downward shear jerk.

## Decision
Implement physical two-handed explosive bolt jettison shear detection in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Both hands positioned in the wing jettison collar zone ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.42]\text{ m}$) with hands braced close together ($dist < 0.25\text{ m}$).
- **Two-Handed Grip & Dynamic Shear**: Both controller grips held firmly ($Grip > 0.60$). Executing a forceful synchronized downward heave ($V_{y, left} < -0.85\text{ m/s}, V_{y, right} < -0.85\text{ m/s}$) triggers the pyrotechnic bolt cutter shears.
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` for interaction, structural detachment, and console acknowledgement.
- **Haptic Feedback**: Dual-controller $280\text{ Hz}$ ($0.98$) pyrotechnic explosive bolt detonation shockwave, followed by $45\text{ Hz}$ ($0.75$) low-frequency structural separation shudder.

## Consequences
- Enables dramatic external EVA hull sabotage and debris clearance during spacewalk sequences.
- Fully compatible with existing EVA mechanics (cold-gas thrusters, magnetic boots, radiation visors).
