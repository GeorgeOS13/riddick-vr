# ADR 0051: Cryo-Pod Cryo-Glass Condensation Wipe & Frost Scraping

## Status
Accepted / Implemented

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*, Riddick begins the story waking inside an automated cryo-hibernation pod aboard the mercenary ship Dark Athena. Throughout the Butcher Bay detention facility and Dark Athena decks, observation ports, containment cells, and cryo-canopies suffer from heavy internal condensation, frost buildup, and cryogenic vapor fogging that severely impede player visibility when attempting to survey adjacent corridors or monitor patrolling mercenary guards.

Traditional flat-screen games rely on fixed scripted camera clears or binary trigger animations. In virtual reality, looking through frosted glass without the ability to physically wipe condensation breaks spatial presence. Players naturally raise their open palms or gauntlets to smear away frost and mist from the transparent canopy.

## Decision
We implement physical cryo-glass condensation wiping and frost scraping in Section 6 of `src/hook/camera_hook.cpp`:
1. **Viewport Proximity & Flat Hand Contact**:
   - Detects hand position within the viewport glass contact zone:
     $Z_{local} \in [-0.40, -0.15]\text{ m}$, $Y_{local} \in [-0.20, +0.25]\text{ m}$.
   - Requires relaxed or flat hand posture ($\text{grip} < 0.60$) to prevent conflict with fist punches or heavy tool grips.
2. **Kinematic Wiping Velocity**:
   - Sweeping the hand laterally or vertically across the glass surface ($|V_x| > 0.40\text{ m/s} \lor |V_y| > 0.40\text{ m/s}$) registers active wiping strokes.
3. **Tactile Friction & Squeak Feedback**:
   - Emits tactile glass friction micro-pulses at $240\text{ Hz}$ ($0.35$ amplitude) throttled to $160\text{ ms}$, replicating rubber gauntlet / skin squeak on cold glass.
4. **Viewport Clearance**:
   - Dispatches spatial interaction clears (`E`), clearing diegetic fog layers and granting unhindered sightlines through observation ports.

## Consequences
- Deepens atmospheric immersion during cryo-pod awakenings and containment cell scouting.
- Operates entirely via spatial kinematic contact without requiring button inputs.
- Prevents false-positive punches or grabs through grip and velocity filtering.
