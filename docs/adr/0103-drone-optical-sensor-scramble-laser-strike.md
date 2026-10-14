# ADR 0103: Dark Athena Drone Optical Sensor Scramble Laser Strike

## Status
Accepted

## Context
Aboard the Dark Athena, cybernetic sentry drones and ceiling-mounted tracker turrets utilize high-sensitivity optoelectronic lenses to detect intruder silhouettes in low-light corridors. When Riddick wields a firearm equipped with a tactical laser aiming module (ADR 0091), shining the concentrated coherent laser diode beam directly into a drone's optical receiver sensor saturates its photodiodes, triggering automatic reboot failsafes and blinding the drone for several seconds.

In standard flat-screen gameplay, blinding mechanics were limited to EMP weapons or smoke grenades. In VR, rewarding steady two-handed laser target alignment against tiny robotic sensor apertures creates a deeply satisfying, skill-based tactical stealth option.

## Decision
1. **Active Laser Module Prerequisite**:
   - Operator must be in stabilized two-handed firearm stance (`s_twoHandedStabilizationActive`) with tactical laser accessory activated (`s_flashlightActive || s_laserActive`).
2. **Optic Alignment & Dwell Time**:
   - Weapon aim ray aligned directly at the sensor target zone ($Z_{rel} \in [-0.45, -0.18]\text{ m}, Y_{rel} \in [-0.10, 0.30]\text{ m}$).
   - Angular alignment stability maintained ($|\vec{\omega}| < 0.20\text{ rad/s}$) for a continuous dwell threshold of $\ge 500\text{ ms}$.
3. **Sensor Bloom & Photodiode Overload Haptics**:
   - Continuous subtle $180\text{ Hz}$ ($0.30$) laser diode resonance in off-hand during dwell, culminating in a sharp $240\text{ Hz}$ ($0.65$) optic sensor overload pop upon successful scramble.
4. **Engine Input Injection**:
   - Injects optical disruption key sequence (`F` + `E`) with a $1600\text{ ms}$ sensor recovery buffer.

## Consequences
- Provides non-lethal, covert options for handling automated security without expending precious ammunition.
- Enforces natural two-handed weapon aiming discipline and precise spatial posture.
