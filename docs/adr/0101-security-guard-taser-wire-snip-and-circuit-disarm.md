# ADR 0101: Security Guard Taser Wire Snip & Circuit Disarm

## Status
Accepted

## Context
Butcher Bay prison guards frequently employ high-voltage electro-stun dart tasers to subdue rebellious inmates from beyond melee reach. Once hooked by copper-wound taser barbs, electrical pulses lock the player's neuromuscular locomotion. Riddick can counter this debilitating hazard by drawing his edged weapon swiftly across the electrified lead wires, severing the conductor cables and grounding the discharge into the floor.

In conventional monitor gameplay, breaking taser holds required repetitive key mashing. In VR, slicing across the chest corridor with an edged blade provides an intuitive, high-adrenaline physical countermeasure.

## Decision
1. **Chest Taser Cable Plane**:
   - Electrified cables span across the front chest plane ($|X_{rel}| < 0.15\text{ m}, Y_{rel} \in [-0.18, 0.10]\text{ m}, Z_{rel} \in [-0.24, -0.05]\text{ m}$).
2. **High-Velocity Lateral Wire Slice**:
   - Dominant hand sweeps across the chest cable corridor with lateral cutting velocity ($|V_x| > 0.90\text{ m/s}$) with weapon grip held ($> 0.50$).
3. **Dielectric Snap & Residual Spark Haptics**:
   - Instantaneous $220\text{ Hz}$ ($0.75$) sharp cable shear snap, followed by a fading $90\text{ Hz}$ ($0.30$) capacitive dissipation fizzle.
4. **Engine Input Injection**:
   - Neutralizes taser stun status (`E` + `3`) with a $1200\text{ ms}$ reset threshold.

## Consequences
- Transforms a frustrating key-mashing stun mechanic into an empowering tactical blade maneuver.
- High velocity and tight chest proximity prevent unintended triggers during normal movement.
