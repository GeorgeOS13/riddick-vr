# ADR 0164: Subterranean Hydroponics Bio-Waste Incinerator Flame Arrestor Mesh Screen Slide-In

## Context
In the Subterranean Hydroponics waste management sectors, explosive combustible methane from bio-sludge poses a critical hazard. To prevent burner backflashes from detonating the upstream chutes, stainless-steel mesh flame arrestor screens must be installed into the burner intake channels.

## Decision
Implement physical two-handed flame arrestor mesh screen insertion via continuous 6DOF motion tracking:
1. **Two-Handed Frame Grasp**:
   - Both hands grasping the screen frame edges:
     - $dist(L, R) < 0.26\text{ m}$.
     - $Z_{controller} \in [-0.40, -0.16]\text{ m}$.
     - $Y_{controller} \in [-0.15, 0.25]\text{ m}$.
     - $Grip_{left} > 0.55 \land Grip_{right} > 0.55$.
2. **Channel Slide-In Thrust**:
   - Sliding the screen frame forward into the intake receiver:
     - $V_{z, right} < -0.80\text{ m/s} \lor V_{z, left} < -0.80\text{ m/s}$.
3. **Synthesized Game Controls**:
   - Synthesizes `E` (Interact) + `Enter` (Lock detent) + `Tab` (Filter channel toggle) for $160\text{ ms}$.
4. **Haptic Feedback**:
   - Dual-hand $190\text{ Hz}$ ($0.65$) wire mesh sliding friction along channel tracks.
   - $260\text{ Hz}$ ($0.90$) mechanical spring seating latch snap.
5. **Cooldown**: $900\text{ ms}$ debounce.

## Consequences
- Complements previous incinerator subsystems (spark wheel, flame damper, blast door levers) into an interconnected puzzle network.
- Uses natural two-handed spatial placement and alignment.
