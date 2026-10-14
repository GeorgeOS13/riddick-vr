# ADR 0058: Security Drone EMP Blast & Stun Overload Strike

## Status
Accepted / Implemented

## Context
Aboard Dark Athena, robotic Ghost Drones and heavy automated security drones stalk hallways. When drones are temporarily stunned by concussion grenades, Tranq gun capacitor darts, or steam valve bursts, Riddick can close distance and execute a devastating cybernetic takedown: physically grabbing the drone's chassis and forcibly torquing the cervical neck assembly to short-circuit the power core and emit a localized EMP shockwave.

## Decision
We implement physical drone grappling and EMP overload snaps in Section 5a/9 of `src/hook/camera_hook.cpp`:
1. **Dual-Hand Chassis Grapple**:
   - Both controllers positioned in front of chest at drone collar height:
     $Z_{local} \in [-0.45, -0.20]\text{ m}$, $Y_{local} \in [-0.25, +0.15]\text{ m}$.
   - Both grips clenched ($> 0.65$) on the chassis frame.
2. **Opposing Torque / Cervical Snap**:
   - Twisting controllers in opposing directions with high angular velocity ($|lVel.y - rVel.y| > 1.20\text{ m/s}$ or $|lVel.x - rVel.x| > 1.20\text{ m/s}$):
   - Snaps the mechanical spine and triggers EMP battery overload (`LMB` + `'E'`).
3. **Electromagnetic Shockwave Haptics**:
   - Emits a dual $0.95$ amplitude shockwave featuring a bone/metal crunch followed by high-frequency $280\text{ Hz}$ EMP arc vibrations across both controllers.

## Consequences
- Rewards aggressive close-quarters combat against mechanical drone adversaries.
- Multi-vector velocity check ensures deliberate two-handed physical torquing.
- EMP shockwave feedback provides satisfying tactile culmination to drone takedowns.
