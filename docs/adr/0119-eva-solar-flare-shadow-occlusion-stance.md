# ADR 0119: Dark Athena EVA Solar Flare Shadow Occlusion Stance

## Status
Accepted

## Context
During exterior hull spacewalks on the Dark Athena, sudden solar flares produce lethal bursts of ionizing radiation. When an emergency lead bulkhead (ADR 0109) is too distant to reach, the player must seek immediate shadow occlusion behind thick structural steel girders, comms masts, or cargo crane towers. 

In VR, physical immersion requires adopting a tight physical tuck stance behind cover, shielding vital biometric organs from the directional stellar ray vector.

## Decision
1. **Physical Tuck Behind Shadow Occlusion**:
   - Player physically crouched or prone ($Y_{HMD} < -0.28\text{ m}$).
   - Both hands drawn inward tightly against the chest/torso corridor ($|X_{local, left}| < 0.14\text{ m} \land |X_{local, right}| < 0.14\text{ m}$, $Z_{local} > -0.20\text{ m}$).
   - Both grips clenched ($Grip_{left} > 0.50 \land Grip_{right} > 0.50$).
2. **Radiation Shielding Confirmation Haptics**:
   - Transitioning into shadow tuck emits a stabilizing cardiovascular heartbeat pulse ($60\text{ Hz}$, $0.35$, $80\text{ ms}$) across both hands.
   - Geiger counter acoustic tick rate is suppressed by $75\%$ while in the tuck.
3. **Engine Input Injection**:
   - Dispatches deep cover occlusion state (`C` + `Space` + `Q`) with a $1500\text{ ms}$ debounce.

## Consequences
- Transforms exterior zero-G solar survival into a physical spatial puzzle of finding and holding tight shadow lines.
- Fully compatible with standard crouching while requiring conscious hand positioning against the body.
