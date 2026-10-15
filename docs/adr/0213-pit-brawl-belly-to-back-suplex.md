# ADR 0213: Inmate Pit Brawl Duck-Under Back-Clinch Belly-to-Back Suplex Slam

## Status
Accepted

## Context
When an adversary throws an over-committed wild hook in pit brawls, Riddick ducks beneath the strike line, pivots behind to establish a tight rear waist lock, and launches a high-amplitude belly-to-back suplex directly onto the concrete floor.

## Decision
Implement physical duck-under back suplex in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: From low duck/crouch ($Y < -0.25\text{ m}$), slip to opponent rear flank ($|rVel.x| > 0.85\text{ m/s}$), locking rear waist clinch ($dist < 0.24\text{ m}$, both grips $> 0.65$), arming a $500\text{ ms}$ suplex execution window.
- **Explosive Arch & Hoist**: Before timeout, violently arch body backward and pull both hands upward-rearward ($lVel.y > 1.20\text{ m/s} \land rVel.y > 1.20\text{ m/s}$) to dump adversary overhead onto their spine.
- **Synthesized Controls**: Emits `S` + `W` + `C` + `Space` + `LMB` for waist lock, backward arch, and high-impact deck slam.
- **Haptic Feedback**: $220\text{ Hz}$ ($0.90$) abdominal lift strain followed by dual $40\text{ Hz}$ ($0.98$) vertebral mat impact blast.

## Consequences
- Elevates counter-grappling depth and rewards rhythmic slipping of enemy strikes.
- Integrates with fireman's carry slams, outside leg sweeps, and front headlocks.
