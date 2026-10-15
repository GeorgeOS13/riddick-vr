# ADR 0181: Inmate Pit Brawl Kimura Shoulder Lock & Arm Fracture

## Status
Accepted

## Context
In brutal close-quarters inmate pit brawls, grappling an opponent's overextended arm allows Riddick to lock a figure-four Kimura grip around their wrist and elbow, cranking the shoulder joint beyond its anatomical limit until bone snaps and the adversary submits or collapses.

## Decision
Implement physical Kimura submission shoulder lock in `src/hook/camera_hook.cpp`:
- **Grapple Alignment**: From active Boxer Guard or clinch range ($dist < 0.25\text{ m}$, $Y \in [-0.20, 0.20]\text{ m}$), grasp the opponent's wrist with the lead hand ($Grip > 0.65$) and wrap dominant forearm over the elbow ($Grip > 0.65$).
- **Rotational Shoulder Crank**: Apply violent rotational torque and upward-inward crank with the dominant hand ($V_{y, right} > 1.05\text{ m/s} \land |V_{x, right}| > 0.85\text{ m/s}$) held for $> 200\text{ ms}$.
- **Synthesized Controls**: Emits `S` + `A` + `LMB` + `Space` for standing submission leverage, shoulder dislocation, and lethal fracture.
- **Haptic Feedback**: $190\text{ Hz}$ ($0.75$) joint bind resistance progressing into an explosive $320\text{ Hz}$ ($0.98$) acromioclavicular and humerus fracture crack.

## Consequences
- Expands Riddick's pit brawl hand-to-hand combat arsenal with high-skill grappling submissions.
- Bridges naturally with existing clinch mechanics (collar tie, underhooks, guillotine, suplex, outside trip).
