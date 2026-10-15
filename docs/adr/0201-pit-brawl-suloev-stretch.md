# ADR 0201: Inmate Pit Brawl Suloev Stretch Hamstring Submission Lock

## Status
Accepted

## Context
In unscripted close-quarters pit brawls, inmates scramble for leverage on the deck. Riddick controls an opponent's back or bottom turtle position, hooking a leg and arching violently to execute the devastating Suloev Stretch hamstring tear and hip dislocation.

## Decision
Implement physical grappling submission lock in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: From low crouch or deep prone ($Y < -0.30\text{ m}$), hook opponent leg with off-hand ($leftLocal.y < -0.35\text{ m}$, Left Grip $> 0.65$) and over-hook the ankle with dominant arm ($Right Grip > 0.65, dist < 0.25\text{ m}$).
- **Arching Stretch**: Violently pull both hands upward and backward ($lVel.y > 1.05\text{ m/s} \land rVel.y > 1.05\text{ m/s}$) to hyper-extend the hamstring and dislocate the hip joint.
- **Synthesized Controls**: Emits `C` + `S` + `Space` + `LMB` for grappling lock, joint over-extension, and submission finish.
- **Haptic Feedback**: $240\text{ Hz}$ ($0.85$) tendon hyper-extension tension, followed by $330\text{ Hz}$ ($0.98$) bone-joint fracture shockwave.

## Consequences
- Deepens the somatic authenticity of the inmate pit brawl grappling ecosystem.
- Pairs with double-leg slams, arm-triangles, Kimuras, and ground-and-pound finishes.
