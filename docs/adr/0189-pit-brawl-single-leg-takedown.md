# ADR 0189: Inmate Pit Brawl Single-Leg Takedown & Ankle Pick Slam

## Status
Accepted

## Context
In pit fighting matches against heavy brawlers, changing levels into a low crouch allows Riddick to dive beneath their punches, wrap the lead knee or calf, pick their heel, and drive through their hips to dump them flat on their back onto the concrete deck.

## Decision
Implement physical level-change ankle pick and single-leg takedown in `src/hook/camera_hook.cpp`:
- **Level Change**: Stance low or physically crouched ($Y < -0.30\text{ m}$).
- **Ankle Pick & Driving Thrust**: Drive lead hand to opponent's heel ($leftLocal.y < -0.45\text{ m}, Left Grip > 0.60, lVel.y > 0.85\text{ m/s}$) while shoving dominant shoulder forward ($rVel.z < -1.10\text{ m/s}$).
- **Synthesized Controls**: Emits `C` + `W` + `Space` + `LMB` for level change penetration, leg trip, and deck slam.
- **Haptic Feedback**: Left $0.80$ ($160\text{ Hz}$) ankle pick scoop impact, Right $0.98$ ($260\text{ Hz}$) deck takedown shockwave.

## Consequences
- Broadens grappling variety with a wrestling-style level change and trip mechanic.
- Synergizes with ground strikes, mount pound, and low-line evasions.
