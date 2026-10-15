# ADR 0139: Dark Athena EVA Solar Collector Truss Angular Lock Pin Shear & Drop

## Status
Accepted

## Context
During EVA spacewalk operations across the exterior hull of the *Dark Athena*, orbital debris and mechanical misalignments can cause the massive articulating solar collector photovoltaic trusses to jam in hazardous extended positions, exposing Riddick to lethal coronal mass ejections or blocking transit routes between airlock hubs. In native gameplay, clearing structural blockages often requires scripted button or panel interactions.

In VR, physical immersion is maximized when structural hull sabotage and emergency mechanical repairs require direct manual tool application or kinetic shear strikes against jammed articulation joints.

## Decision
We implement **EVA Solar Collector Truss Angular Lock Pin Shear & Drop**:
1. **Spatial Joint Corridor**:
   - Dominant hand positioned at the truss pivot collar ($Z \in [-0.42, -0.15]\text{ m}$, $Y \in [0.10, 0.45]\text{ m}$).
   - Dominant grip clenched ($> 0.60$) bracing against the shear pin locking sleeve.
2. **Kinetic Shear Strike**:
   - Delivering a sharp downward kinetic hammer strike or forceful downward jerk ($V_y < -0.85\text{ m/s}$) shears the binding alloy shear pin.
3. **Input Dispatch**:
   - Dispatches `E` (interact) + `Space` (unclamp / hinge release) + `Tab` (objective update / stow sequence).
4. **Haptic Feedback**:
   - Dominant hand receives a $0.95$ metallurgical fracture shockwave at $260\text{ Hz}$ ($120\text{ ms}$), with a $0.60$ structural resonance recoil vibration across the off-hand.

## Consequences
- Allows tactile mechanical sabotage of massive hull structures during spacewalks.
- Natural extension of the EVA physical interaction suite alongside solar scrape, cable tensioning, and airlock dogging.
- Fully debounced with a $1500\text{ ms}$ refractory period to avoid double dispatch.
