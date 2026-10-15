# ADR 0150: Security Guard Stun Baton Ground Pin & Taser Kneel

## Status
Accepted

## Context
In encounters against armored guards or aggressive cybernetic drones that have been staggered, swept, or knocked down to the deck grating, allowing them time to recover invites renewed counter-attacks. Riddick can drop down into a physical knee-pin while plunging an energized stun baton vertically into their chest or spine, anchoring their torso against the steel floor while dumping continuous high-voltage discharge directly through their armor into the ship's ground circuit.

In VR, dropping into a physical crouch or kneel while plunging a weapon vertically into the floor plane provides authentic bodily immersion and tactical dominance.

## Decision
We implement **Security Guard Stun Baton Ground Pin & Taser Kneel**:
1. **Low Posture & Ground Plane Alignment**:
   - Player is physically crouched or kneeling ($Y_{HMD} < -0.30\text{ m}$).
   - Dominant baton controller is low near the floor ($Y < -0.45\text{ m}$).
   - Both grip and trigger firmly held ($Grip > 0.60 \land Trigger > 0.65$).
2. **Vertical Ground Plunge**:
   - Driving the baton downward into the floor ($V_{dominant, y} < -1.15\text{ m/s}$).
3. **Input Dispatch**:
   - Dispatches `C` (crouch/kneel pin) + `S` (ground hold) + `Q` (electrostatic drain) + `LMB` (primary attack).
4. **Haptic Feedback**:
   - Dominant hand delivers a bone-jarring $0.98$ deck collision thud at $60\text{ Hz}$, followed by a $0.85$ continuous grounding arc buzz at $260\text{ Hz}$.

## Consequences
- Rewarding follow-up after low sweeps, shoves, or parries.
- Debounced with a $1500\text{ ms}$ refractory cooldown.
