# ADR 0199: Dark Athena EVA Solar Collector Truss Articulation Lock Ring Pinch Pin Insertion

## Status
Accepted

## Context
Securing the solar collector truss articulation lock ring against orbital thermal expansion cycling requires driving a spring-loaded pinch pin into the retaining collar bore. Riddick stabilizes the alignment guide with his off-hand and drives the pinch pin into the retaining collar with a linear tool thrust.

## Decision
Implement physical two-handed pinch pin insertion in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position off-hand on the alignment guide sleeve ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$) and dominant hand holding the pinch pin ($dist < 0.22\text{ m}$, Right Grip $> 0.60$).
- **Linear Drive Thrust**: Deliver a high-velocity forward thrust along the bore axis ($rVel.z < -0.90\text{ m/s}$) to seat the pin through the collar.
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Enter` for tool alignment, pin advancement, and detent seating.
- **Haptic Feedback**: $190\text{ Hz}$ ($0.65$) guide sleeve friction followed by $290\text{ Hz}$ ($0.98$) spring-loaded detent lock click.

## Consequences
- Solidifies mechanical EVA repair mechanics during exterior shipboard navigation.
- Pairs seamlessly with spanner wrenches, alignment drift pins, and mallet strikes.
