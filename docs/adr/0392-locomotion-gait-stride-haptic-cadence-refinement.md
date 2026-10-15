# ADR 0392: Locomotion Gait Stride Haptic Cadence Refinement

## Context
Previously, ADR 0040 implemented magnetic boot stride cadence that pulsed the controller haptics at a high amplitude of $0.35$ every 520ms whenever any locomotion key was active.
During normal corridor navigation and room exploration, this caused persistent hand numbness and tactile fatigue, obscuring combat cues and weapon recoil sensations.

## Decision
1. Scale down walking stride haptics to a subtle, unobtrusive $0.06$ tactile gait click, alternating between left and right controllers every 540ms.
2. When actively sprinting (ADR 0391), accelerate the stride interval to 360ms with a brisk $0.12$ momentum pulse.
3. Completely suppress walking stride haptics during stealth crouching, creeping, or prone crawls.

## Consequences
- Prevents player hand fatigue across extended VR play sessions.
- Preserves dynamic range and sensory headroom for high-impact combat haptics, gunshots, and parry feedback.
