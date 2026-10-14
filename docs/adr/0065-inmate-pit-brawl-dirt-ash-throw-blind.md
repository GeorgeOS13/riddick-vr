# ADR 0065: Inmate Pit Brawl Dirt / Ash Throw & Blind

## Status
Accepted / Implemented

## Context
During pit brawls in Butcher Bay's lower mining levels and furnace boiler rooms, inmates resort to dirty street fighting tactics to overcome armored guards and predatory inmates. Gathering gravel, sand, or searing ash from the deck and tossing it directly into an adversary's ocular sensors or eyes temporarily disorients them, opening up windows for throat-slit executions or heavy power slams.

In VR, crouching low to scoop loose particulates followed by an underhand or overhand flick forward creates an intuitive, tactile dirty-fighting mechanic.

## Decision
We implement physical dirt/ash gathering and throwing in Section 5a/9 of `src/hook/camera_hook.cpp`:
1. **Low Deck Gathering**:
   - Player physically crouched or reaching to floor level:
     $Y_{local} < -0.55\text{ m}$.
   - Squeezing Left Grip scoops particulate matter, confirmed by a subtle $140\text{ Hz}$ ($0.30$) grit granule rustle.
2. **Kinematic Ash Cloud Flick**:
   - Upward and forward arm fling:
     $V_{y, left} > 0.80\text{ m/s} \land V_{z, left} < -0.80\text{ m/s}$.
   - Dispatches temporary blinding distraction signal (`E` + `Q`), staggering nearby enemies.
3. **Particulate Burst Haptics**:
   - Emits a crisp $180\text{ Hz}$ ($0.55$ amplitude) granular scatter impulse upon release.

## Consequences
- Broadens tactical options in unarmed or melee combat encounters.
- Distinct upward/forward trajectory prevents confusion with downward object drops or forward straight punches.
- Enhances player agency during high-difficulty pit brawls.
