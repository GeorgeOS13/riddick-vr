# ADR 0374: Quest Controller Ergonomic Barrel Pitch Calibration & Sight Alignment

## Status
Accepted

## Context
Standard OpenXR runtime `/user/hand/right/input/aim/pose` tracking represents a ray projecting along the physical controller body. On Meta Quest (Quest 2, Quest 3, Quest Pro) and similar Touch-style controllers, holding the grip with a natural, relaxed wrist posture causes the raw aim ray to point approximately $10^\circ$ to $12^\circ$ upward relative to the natural bore axis of firearms.

In VR shooter conversions, forcing the player to continually flex their wrist downward to keep gun sights leveled causes wrist fatigue during extended play sessions.

## Decision
Incorporate an ergonomic pitch angle correction into the controller aim processing pipeline in `src/hook/camera_hook.cpp`:

1. **Diegetic Pitch Bias Offset**:
   - Introduce an ergonomic downward pitch offset:
     $$\text{ctrlPitch} \mathrel{+}= \theta_{\text{offset}} \quad (\theta_{\text{offset}} \approx -10.5^\circ \approx -0.183\text{ rad})$$
   - This aligns the virtual weapon barrel and iron sights directly with the user's natural forearm index line when the hand is at rest.

2. **ADS Alignment**:
   - When raising the weapon to eye height for Aim-Down-Sights (ADS), the calibrated barrel axis aligns naturally with the HMD line of sight without requiring awkward wrist articulation.

## Consequences
- Significant reduction in wrist strain during prolonged combat.
- Natural pointing accuracy and intuitive hip-fire / point-shooting alignment.
