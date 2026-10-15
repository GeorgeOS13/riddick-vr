# ADR 0371: Thumbstick Snap vs Smooth Turning Ergonomics & Input Hysteresis

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena* VR mod, horizontal thumbstick rotation on the right controller previously combined snap turning and smooth turning in a single non-isolated code branch (`src/hook/camera_hook.cpp`). When a player deflected the stick beyond the deadzone (0.25), smooth mouse movement was sent continuously (`stickX * 16.0f`). If the deflection exceeded 0.75, a discrete snap turn (350 mouse delta) was triggered.

This created a severe ergonomic defect:
1. When performing a quick thumb flick to snap turn, the thumbstick invariably spent 1–3 frames transitioning through the 0.25–0.75 range, causing an unintended smooth crawl right before the snap occurred.
2. When releasing the thumbstick back to center, the stick again traversed the 0.75–0.25 range, applying post-snap smooth turning.
3. Players who preferred smooth turning suffered sudden, jarring 45° snap jumps whenever the stick hit full deflection.
This mixed behavior is a primary cause of simulator sickness in VR.

## Decision
Refactor right thumbstick rotation in `src/hook/camera_hook.cpp` to decouple Snap Turn and Smooth Turn into distinct, configurable modes:

1. **Turn Mode Toggle (`VRTurnMode`)**:
   - `VR_TURN_SNAP`: Discrete 45° snap turns (default for comfort).
   - `VR_TURN_SMOOTH`: Continuous rotation with cubic acceleration curve.
   - User toggle: Holding Left Grip while clicking Right Thumbstick switches between modes.
   - Haptic acknowledgment: Distinct high-frequency confirmation pulse upon switching.

2. **Snap Turn Hysteresis (`VR_TURN_SNAP`)**:
   - Snap threshold: Requires stick deflection past `|stickX| > 0.70f`.
   - Once triggered, dispatches exactly one snap delta (`±350` mouse units) and provides a crisp tactile click (`0.25f` haptic pulse).
   - Re-arming hysteresis: The stick must return below `|stickX| < 0.30f` before another snap turn can be executed, preventing unintentional double-snapping.
   - **Crucial Invariant**: In Snap Turn mode, zero continuous mouse movements are dispatched in the sub-threshold region (0.0 to 0.70). Pre-snap and post-snap crawl is completely eliminated.

3. **Smooth Turn Cubic Response (`VR_TURN_SMOOTH`)**:
   - Deadzone: `0.20f`.
   - Response curve: Normalized stick deflection scaled via a progressive power curve:
     $$\text{speed} = \left(\frac{|\text{stickX}| - 0.20}{0.80}\right)^2 \times 18.0$$
   - Micro-aim precision: At low deflections, speed scales smoothly to single-pixel deltas; at outer deflections, fast turning is achieved without discontinuous jumps.
   - Zero snap jumps are dispatched at full deflection.

## Consequences
- Completely eliminates pre-snap crawl and post-snap drift during thumbstick flicking.
- Provides players full control to choose between motion-sickness-free snap turning and smooth turning.
- Preserves native game sensitivity scaling without introducing latency or frame drops.
