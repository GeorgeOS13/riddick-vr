# ADR 0375: Virtual Cinema Mode Lazy-Follow Yaw Dynamics & Recenter Ergonomics

## Status
Accepted

## Context
In *The Chronicles of Riddick* VR mod, menus, HUDs, and non-interactive cutscenes are rendered onto a floating 16:9 virtual cinema screen (`XrCompositionLayerQuad`) at a distance of 1.8 meters in front of the player (`src/host/openxr_manager.cpp`).

Previously, the world-space pose of the cinema quad was calculated only once when cinema mode was initiated. If the user rotated their physical body or turned their head past 45° while browsing menus, dialogue trees, or watching extended cinematics, the screen remained statically anchored at the old heading. This required the user to either turn back awkwardly or click the thumbstick to re-center.

## Decision
Implement a smooth "lazy-follow" heading tracking system for the virtual cinema quad in `src/host/openxr_manager.cpp`:

1. **Angular Yaw Deviation Monitoring**:
   - Every frame during active cinema mode, calculate the angular difference $\Delta \psi$ between the current HMD view yaw and the cinema screen anchor yaw:
     $$\Delta \psi = \psi_{\text{HMD}} - \psi_{\text{screen}}$$
   - Normalize $\Delta \psi$ to $[-\pi, \pi]$.

2. **Deadband & Smooth Slerp Follow**:
   - Establish a comfortable viewing deadband cone ($\pm 40^\circ \approx \pm 0.70\text{ rad}$).
   - While the player's gaze stays within the deadband ($|\Delta \psi| \le 0.70$), the screen remains rock-solid in world space to preserve stable reading posture.
   - When the gaze exceeds the threshold ($|\Delta \psi| > 0.70$), the screen smoothly glides toward the player's new line of sight at an exponential rate ($\alpha = 0.05$), bringing the display gently back into comfortable view without disorienting snapping.

3. **Instant Recenter**:
   - Explicit recenter requests (`clientState.shouldRecenter` or manual recenter) immediately snap the screen heading and position directly in front of the player's current view.

## Consequences
- Prevents neck strain during extended dialogue sequences and menu navigation.
- Preserves rock-steady reading stability when looking around the menu borders while smoothly accommodating large physical body rotations.
