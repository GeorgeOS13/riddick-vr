# ADR 0059: High-Voltage Security Laser Mesh Spatial Deflection

## Status
Accepted / Implemented

## Context
Butcher Bay's Facility Double-Max and Emperor Station contain high-density infrared laser tripwire meshes that sweep corridors. Touching any laser emitter immediately triggers automated sentry turrets, blast door lockdowns, and alarm klaxons. While Deep Prone Limbo (ADR 0030) enables crawling beneath static low beams, vertical sweeping grids cannot be bypassed on foot without cover.

In virtual reality, holding a ballistic riot shield or heavy metal plate directly into the path of an infrared laser beam should allow Riddick to intercept and deflect the optical beam, creating a momentary shadow corridor to traverse without tripping alarms.

## Decision
We implement laser beam spatial deflection in Section 6b of `src/hook/camera_hook.cpp`:
1. **Deflection Shield Stance**:
   - Requires two-handed weapon/shield stabilization (`s_twoHandedStabilizationActive`).
   - Shield plane held directly forward in front of chest/face:
     $Z_{local} \in [-0.35, -0.15]\text{ m}$, $|Y_{local}| < 0.20\text{ m}$.
2. **Optical Scatter Haptic Feedback**:
   - Emits continuous rapid $220\text{ Hz}$ ($0.30$ amplitude) laser refraction micro-vibrations across both controllers while deflecting beams.
3. **Safe Passage Corridor**:
   - Suppresses proximity alarm triggers (`VK_TAB` stealth damping) while shield plane remains steady ($|V| < 0.35\text{ m/s}$).

## Consequences
- Provides tactical room-scale VR solutions for navigating dense dynamic security laser grids.
- Requires steady two-handed composure under intense environmental pressure.
- Prevents alarm triggering during synchronized shield advancements.
