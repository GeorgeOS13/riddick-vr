# ADR 0154: Security Guard Stun Baton Two-Handed Cross-Chest Defensive Arc Wall

## Status
Accepted

## Context
When facing multiple armed adversaries or sustained automatic gunfire in narrow prison corridors, Riddick can hold a captured riot baton horizontally across his chest with both hands gripping either end while depressing both capacitor switches. This projects a localized planar electrostatic reactive shield that dissipates kinetic projectile energy and electrocutes lunging melee attackers.

In VR, physical two-handed chest barrier postures provide an intuitive, heroic defensive stance against suppression fire.

## Decision
We implement **Security Guard Stun Baton Two-Handed Cross-Chest Defensive Arc Wall**:
1. **Horizontal Cross-Chest Geometry**:
   - Hands spaced across chest width ($|X_{dominant} - X_{offhand}| > 0.22\text{ m}, dist \in [0.20, 0.38]\text{ m}$).
   - Held across chest corridor ($Y \in [-0.20, 0.10]\text{ m}$, $Z \in [-0.30, -0.10]\text{ m}$).
   - Both grips clenched ($> 0.65$) with both triggers held ($> 0.60$).
2. **Arc Wall Activation**:
   - Holding this braced posture projects the electrostatic arc wall continuously.
3. **Input Dispatch**:
   - Dispatches `RMB` (block mode) + `Q` (electrostatic barrier) + `S` (recoil anchor).
4. **Haptic Feedback**:
   - Sustained dual $240\text{ Hz}$ ($0.65$) electrostatic repulsion hum, spiking to $0.98$ upon projectile or melee deflection.

## Consequences
- Complements the offensive spear thrust (ADR 0146) and ground pin (ADR 0150) with a specialized heavy defensive stance.
- Seamlessly transitions into and out of Boxer Guard.
