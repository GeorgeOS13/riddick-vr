# ADR 0400: Broadened Boxer Guard Detection Volume & Grip Removal

## Context
1. Players holding controllers up in front of their face to block reported that blocking did not reliably engage.
2. Root causes identified:
   - **Strict Grip Squeeze Requirement:** Guard pose previously required `fistsClenched = (vrState.leftController.grip > 0.40f && vrState.rightController.grip > 0.40f)`. When players naturally raised their arms in front of their face or chin without continuously squeezing both grip triggers, the guard failed to trigger.
   - **Overly Restrictive Forward Depth Corridor:** Distance in front of the headset was constrained to $Z \in [-0.42, -0.12]\text{ m}$. Natural human arm shield posture with partially extended forearms often places controllers 45–55 cm from the headset ($Z \in [-0.55, -0.45]\text{ m}$), falling outside the corridor.
   - **Narrow Horizontal Hand Separation:** Hands were required to be within 35 cm of each other ($|X_{\text{left}} - X_{\text{right}}| < 0.35\text{ m}$), failing when hands were positioned at natural shoulder width (40–48 cm).

## Decision
1. **Broaden Guard Pose Detection Envelope:**
   - Elevation range relative to HMD eye level: $Y \in [-0.36, +0.25]\text{ m}$ (covers chest, chin, and eye guard elevations).
   - Forward depth corridor: $Z \in [-0.58, -0.10]\text{ m}$ (accommodates both tight chin guards and extended forearm shields).
   - Hand separation span: $|X_{\text{left}} - X_{\text{right}}| < 0.48\text{ m}$ (covers natural athletic shoulder-width guard).
2. **Remove Strict Grip Squeeze Prerequisite:**
   - Raising both controllers into the defensive facial volume engages block regardless of grip trigger pressure.
3. **Temporal Debouncing & Punch Release:**
   - 50ms hold hysteresis to engage block (`mouse_event(MOUSEEVENTF_RIGHTDOWN)`).
   - Instant 0ms release when forward punch velocity exceeds $1.4\text{ m/s}$ so jabs and hooks fire without delay.
   - 80ms release buffer when hands lower naturally to prevent boundary jitter.
   - 0.40 amplitude haptic confirmation pulse on both controllers when block engages.

## Consequences
- Raising hands in front of the chin or face immediately and reliably locks into defensive blocking.
- Punching out of guard is crisp and instantaneous.
- Two-handed firearm stabilization is properly suppressed while in boxer guard.
