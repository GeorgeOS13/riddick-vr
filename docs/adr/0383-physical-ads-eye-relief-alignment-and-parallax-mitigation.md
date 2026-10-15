# ADR 0383: Physical 6DOF Aim-Down-Sights (ADS) Eye-Relief Alignment & Parallax Mitigation

## Status
Accepted

## Context
In first-person VR shooters, players instinctively raise firearms up to eye level to sight along the weapon's barrel and iron sights. In *The Chronicles of Riddick* VR mod, aiming state was previously driven solely by the Left Trigger or general two-handed stabilization. When raising a one-handed pistol or knife to eye level without holding the Left Trigger, the aim tracking remained in wide hip-fire sensitivity mode (`wAim = 0.35f`).

Furthermore, bringing the weapon controller within 15–30 cm of the HMD causes slight physiological wrist roll oscillations to create significant optical parallax between the front sight post and rear sight notch, resulting in inaccurate aiming.

## Decision
Implement a diegetic physical Aim-Down-Sights (ADS) corridor and optical parallax mitigation system in `src/hook/camera_hook.cpp`:

1. **Dominant Eye-Relief Bounding Corridor**:
   - Track weapon controller coordinates in HMD-relative view space:
     $$Y \in [-0.18, 0.08]\text{ m (chin to eye level)}$$
     $$Z \in [-0.36, -0.10]\text{ m (10 cm to 36 cm eye relief)}$$
     $$X \in [0.02, 0.22]\text{ m (dominant right eye sight line corridor)}$$

2. **Latching & Hysteresis**:
   - Require holding the controller within this bounding corridor for $> 50\text{ ms}$ before engaging ADS.
   - Fire a subtle $0.18$ amplitude haptic detent pulse ($25\text{ ms}$) on the dominant hand when the sight picture is acquired.
   - Disengage ADS only when moving clearly outside the extended volume ($Y < -0.24\text{ m} \lor Z < -0.42\text{ m} \lor X < -0.05\text{ m} \lor X > 0.28\text{ m}$).

3. **Sight Stabilization & Parallax Mitigation**:
   - While in physical ADS, ramp aim steering weight to $wAim = 0.90f$ and $wPull = 0.12f$ for fine millimeter target tracking.
   - Increase look damping by $50\%$ to suppress micro-tremor.
   - Damp roll oscillations relative to the HMD horizon line to keep iron sights squarely aligned.

## Consequences
- Fluid, natural transition to iron sights simply by raising the controller to eye line.
- Tactile haptic feedback gives immediate confirmation that the sight line is locked on target.
- Eliminates optical parallax wobble and improves long-range headshot precision.
