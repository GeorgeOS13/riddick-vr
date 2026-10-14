# ADR 0098: Dark Athena EVA Micro-Thruster Jetpack Burst

## Status
Accepted

## Context
During EVA spacewalks on the exterior hull of the Dark Athena, structural damage leaves wide open chasms where magnetic boot soles (ADR 0040) cannot maintain surface adhesion. To traverse deep-space hull gaps and leap over security searchlight corridors, Riddick can engage emergency compressed cold-gas micro-thrusters mounted on the hips of his EVA harness.

In flat desktop gameplay, exterior traversal across chasms relied on magnetic boots or fixed ziplines. In VR, extending both arms backward along the hips and squeezing both triggers simultaneously fires twin cold-gas micro-jets, providing intuitive physical propulsion across zero-G voids.

## Decision
1. **Hip Jet Trajectory & Backward Arm Extension**:
   - Both hands positioned rearward alongside hips/torso:
     - Longitudinal depth: $Z_{left} > -0.06\text{ m} \land Z_{right} > -0.06\text{ m}$ (behind chest plane).
     - Lateral spacing: $|X_{left}| > 0.16\text{ m} \land |X_{right}| > 0.16\text{ m}$.
     - Vertical height: $Y_{left} \in [-0.35, -0.05]\text{ m} \land Y_{right} \in [-0.35, -0.05]\text{ m}$.
2. **Dual-Trigger Thruster Ignition**:
   - Both triggers depressed simultaneously ($Trigger_{left} > 0.65 \land Trigger_{right} > 0.65$).
3. **Pneumatic Gas Thrust & Valve Vibration**:
   - Continuous high-velocity cold-gas jetting synthesizes sustained $130\text{ Hz}$ ($0.65$) pneumatic thrust rumble across both controllers for the duration of the burn.
4. **Engine Input Injection**:
   - Injects forward zero-G thrust locomotion sequence (`Space` + `W`) with a $300\text{ ms}$ cadence while triggers are held.

## Consequences
- Provides thrilling physical locomotion during orbital spacewalk sequences.
- Arm-behind-hip posture prevents accidental activation during standard firearm aiming or melee combat.
