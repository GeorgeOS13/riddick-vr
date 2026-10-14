# ADR 0078: Dark Athena EVA Airlock Manual Handwheel Decompression Dogging

## Status
Accepted

## Context
During spacewalk excursions on the outer hull of the Dark Athena, Riddick navigates between exterior airlock vestibules and vacuum environments. Before cycling airlock pressure, massive rotary door dogging spindles must be cranked shut to clamp hermetic gasket seals against the vacuum of deep space.

In flat desktop play, sealing airlocks was executed by single button taps. In VR, locking life-or-death EVA hatches requires physical rotary arm motion.

## Decision
We implement physical dual-handed airlock dogging wheel cranking:
1. **Spatial Proximity Envelope:**
   - Both hands positioned at the heavy bulkhead wheel hub ($Z_{local} \in [-0.42, -0.16]\text{ m}, Y_{local} \in [-0.20, +0.25]\text{ m}$).
   - Both controller grips engaged ($Grip_{left} > 0.50 \land Grip_{right} > 0.50$).
2. **Synchronized Tangential Rotation:**
   - Tangential angular speed of controllers exceeds $0.55\text{ m/s}$ in circular motion.
   - Requires completing 3 successive circular dogging cycles ($now - s\_lastAirlockCrankTime > 220\text{ ms}$).
3. **Engine Action Dispatch & Hermetic Seal Haptics:**
   - On completion of 3 dogging revolutions, dispatches interaction keystroke (`E`) to dog the hatch and trigger airlock repressurization.
   - Generates progressive ratchet resistance clicks ($0.50$ at $160\text{ Hz}$) culminating in a heavy hermetic clamp thud and pneumatic seal hiss ($0.85$ amplitude at $100\text{ Hz}$).

## Consequences
- Elevates the atmosphere and physical vulnerability of EVA spacewalk sequences.
- Multi-revolution requirement mirrors the physical weight and mechanical leverage of vacuum-rated pressure bulkheads.
