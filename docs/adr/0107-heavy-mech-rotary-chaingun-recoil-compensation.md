# ADR 0107: Heavy Mech Rotary Chaingun Dual-Hand Recoil Compensation

## Status
Accepted

## Context
In *Assault on Dark Athena*, combat mechs and stationary automated defense turrets feature multi-barrel rotary chainguns capable of devastating fire rates (ADR 0024). In VR, sustained cyclic fire generates violent barrel climb and angular jitter that disrupts long-range aim. 

By allowing the player to physically cup their off-hand beneath the primary firing hand or barrel shroud—forming a rigid two-handed combat brace—we introduce physical recoil compensation that dampens angular muzzle climb and tightens shot dispersion.

## Decision
1. **Two-Handed Chaingun Brace Detection**:
   - Both motion controllers within close proximity ($dist < 22\text{ cm}$).
   - Dominant firing trigger fully depressed ($Trigger_{right} > 0.80$) while off-hand foregrip is clenched ($Grip_{left} > 0.60$).
2. **Kinematic Recoil Compensation**:
   - While actively braced during sustained cyclic fire (burst duration $> 300\text{ ms}$):
     - Muzzle climb velocity dampening applied ($\alpha_{pitch} = 0.35$).
     - Angular aim smoothing tightened to eliminate micro-jitter.
3. **Harmonic Dual-Hand Haptic Transmission**:
   - Dominant hand: $160\text{ Hz}$ ($0.65$) mechanical bolt cycling.
   - Off-hand foregrip: $110\text{ Hz}$ ($0.70$) heavy hydraulic chassis counter-torque absorption.
4. **Engine Input Injection**:
   - Injects counter-recoil micro-offsets into mouse delta inputs (`deltaPitch` and `deltaYaw`) to actively stabilize the weapon crosshair.

## Consequences
- Rewards players who adopt realistic tactical two-handed stances when piloting mechs or firing heavy rotary chainguns.
- Fully compatible with single-handed hip-fire while offering distinct mechanical benefits for dual-hand stabilization.
