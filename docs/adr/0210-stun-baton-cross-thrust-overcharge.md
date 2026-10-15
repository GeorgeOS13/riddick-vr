# ADR 0210: Security Guard Stun Baton Forward Two-Handed Cross-Chest Thrust & Overcharge Detonation

## Status
Accepted

## Context
When encountering heavily armored heavy guards or security mechs, a standard stun discharge may fail to penetrate the armor plating. Riddick holds an energized cross-chest stance for extended capacitive saturation before lunging forward with explosive force to detonate a maximum-voltage overcharge shockwave.

## Decision
Implement physical two-handed overcharge thrust in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Hold cross-chest guard stance ($handsDist \in [0.20, 0.38]\text{ m}, |\Delta X| > 0.22\text{ m}$, both grips $> 0.65$, both triggers $> 0.65$) for $> 500\text{ ms}$ dwell.
- **Overcharge Forward Lunge**: Explode forward with high dual linear velocity ($rVel.z < -1.30\text{ m/s} \land lVel.z < -1.10\text{ m/s}$) to discharge the overcharged capacitor bank.
- **Synthesized Controls**: Emits `RMB` + `W` + `Space` + `Q` + `Enter` + `LMB` for full capacitor discharge, armor-penetrating arc, and concussive knockdown.
- **Haptic Feedback**: $260 \to 340\text{ Hz}$ ($0.75 \to 0.95$) escalating supercharge hum followed by dual $360\text{ Hz}$ ($0.98$) dielectric breakdown detonation shockwave.

## Consequences
- Rewards tactical patience and defensive discipline with maximum destructive output.
- Complements standard cross-thrust disruption, cross-parry ripostes, and charged skull breakers.
