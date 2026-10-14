# ADR 0073: Inmate Pit Brawl Dirt / Ash Eye Recovery Blink Gesture

## Status
Accepted

## Context
During pit brawls and close-quarters prison scuffles, inmates throw grit, soot, and ash (as codified in ADR 0065). When Riddick is hit with blinding dust or concussive flashbangs, visual acuity is compromised by particulate matter on the corneal surface.

In flat desktop games, players could only wait out the blinding visual overlay. In VR, natural human defensive reflexes include rapid head shaking and wiping the eyes with the forearm or back of the hand to dislodge foreign debris.

## Decision
We implement a physical corneal recovery wipe and rapid head-shake gesture:
1. **Kinematic Reflex Detection:**
   - **Method A (Forearm / Hand Wipe):** Either controller swept horizontally across the eye line ($Z_{local} \in [-0.22, -0.05]\text{ m}, Y_{local} \in [-0.05, +0.10]\text{ m}$) with lateral hand velocity $|V_x| > 0.50\text{ m/s}$.
   - **Method B (Rapid Head Shake):** Angular velocity of the HMD exceeding $2.8\text{ rad/s}$ ($|\Delta Yaw| > 0.15\text{ rad}$ per frame).
2. **Action Dispatch & Recovery Acceleration:**
   - Dispatches visual clearance trigger (`VK_SPACE` tap / UI dismiss) to immediately accelerate corneal clear rate by $2.0\times$.
   - Generates delicate corneal blink micro-haptics across both controllers ($0.30$ amplitude at $180\text{ Hz}$).

## Consequences
- Engages the player's natural human reflexes during blinding combat situations.
- Complements ADR 0065 (dirt/ash throw) by providing an active physical counterplay mechanism.
