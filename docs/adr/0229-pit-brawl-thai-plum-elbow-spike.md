# ADR 0229: Inmate Pit Brawl Clinch Thai Plum Elbow Spike & Cranial Fracture

## Status
Accepted

## Context
Controlling an adversary via the Thai Plum collar tie (ADR 0217) breaks their posture and exposes their crown. Pulling the collar tight while plunging a heavy 12-to-6 downward elbow spike fractures the cranium and concusses the opponent instantly.

## Decision
Implement physical Thai Plum elbow spike in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: In clinch range ($dist < 0.25\text{ m}$), clamp off-hand behind opponent neck ($leftLocal.y > -0.10\text{ m}$, Left Grip $> 0.65$) while raising dominant hand overhead ($rightLocal.y > 0.15\text{ m}$, Right Grip $> 0.65$).
- **Downward Elbow Plunge**: Plunge dominant elbow downward violently ($rVel.y < -1.30\text{ m/s}$) while jerking off-hand inward ($lVel.z > 0.60\text{ m/s}$).
- **Synthesized Controls**: Emits `W` + `A`/`D` + `Space` + `LMB` for collar jerk, downward diagonal elbow trajectory, and cranial fracture.
- **Haptic Feedback**: $220\text{ Hz}$ ($0.85$) cervical collar tension followed by $350\text{ Hz}$ ($0.98$) cranial bone fracture shockwave.

## Consequences
- Broadens lethal Muay Thai infighting finishes in inmate brawls.
- Complements Thai plum liver knees, headlocks, and collar tie headbutts.
