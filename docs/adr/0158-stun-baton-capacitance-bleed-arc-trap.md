# ADR 0158: Security Guard Stun Baton High-Voltage Capacitance Bleed Arc Trap

## Status
Accepted

## Context
When evading pursuing guard squads or setting up defensive ambushes in dark subterranean corridors, Riddick can physically contact the tip of an energized security baton against exposed metal conduit junctions, deck frames, or door surrounds. By holding both triggers to bleed the capacitor bank into the conductive framework, he creates a localized electrostatic trap that electrifies the threshold and incapacitates the next adversary who steps across.

In VR, resting the weapon tip against an environmental surface and feeling the charge transfer into the deck adds tactical stealth preparation.

## Decision
We implement **Security Guard Stun Baton High-Voltage Capacitance Bleed Arc Trap**:
1. **Conduit Surface Contact Envelope**:
   - Two-handed baton grip ($dist(Left, Right) < 0.22\text{ m}$).
   - Baton tip held steady against environmental surface ($Z \in [-0.42, -0.15]\text{ m}, Y \in [-0.30, 0.10]\text{ m}$, $|\vec{V}| < 0.12\text{ m/s}$).
2. **Capacitor Bleed Dwell**:
   - Squeezing both triggers ($> 0.70$) for $> 400\text{ ms}$ discharges capacitor current into the metal surface.
3. **Input Dispatch**:
   - Dispatches `E` (trap prime) + `Q` (electrostatic surge) + `Enter` (conduit arming).
4. **Haptic Feedback**:
   - Ascending $280\text{ Hz}$ capacitive transfer buzz ($0.40 \to 0.90$) culminating in an assertive $140\text{ Hz}$ ($0.95$, $150\text{ ms}$) static surface detonation snap.

## Consequences
- Expands tactical ambush possibilities with the stun baton.
- Fully debounced with a $2000\text{ ms}$ cooldown.
