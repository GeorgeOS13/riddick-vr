# ADR 0089: Security Camera Thermal Decoy Flare Igniting Gesture

## Status
Accepted

## Context
Butcher Bay high-security facilities and Dark Athena sentry corridors employ automated infrared thermal cameras and heat-tracking turret sensors. Deploying high-output magnesium emergency flares creates an intense infrared blooming field that blinds optical tracking systems, scrambles thermal sensors, and illuminates subterranean chasms.

In the 2D original title, flares or throwables were bound to inventory keys or generic grenade slots. In VR, striking an industrial friction flare requires gripping the cylindrical flare body with the off-hand, grasping the abrasive striker cap with the dominant hand, and executing a violent shearing draw stroke across the ignition primer.

## Decision
1. **Bimanual Flare Proximity & Grasp**:
   - Off-hand holding flare body with Left Grip held ($> 0.55$) in front of chest ($Z_{left} \in [-0.35, -0.15]\text{ m}$).
   - Dominant hand positioned at striker cap tip ($|\vec{P}_{dom} - \vec{P}_{off}| < 16\text{ cm}$) with dominant grip held ($> 0.55$).
2. **Abrasive Striker Shear Stroke**:
   - Detect rapid outward or downward shearing stroke velocity between hands ($|\Delta V_y| > 0.80\text{ m/s}$ or $|\Delta V_x| > 0.80\text{ m/s}$) simulating ripping the friction cap across the phosphorus match-head.
3. **Percussion Flash & Roaring Thermal Haptics**:
   - Sharp $260\text{ Hz}$ ($0.85$) striker percussion snap in both hands ($45\text{ ms}$), transitioning immediately into continuous $90\text{ Hz}$ ($0.70$) thermal combustion hiss and magnesium sputtering crackle.
4. **Engine Input Injection**:
   - Equips and ignites flare decoy item (`4` + `LMB`) with an $1800\text{ ms}$ re-ignition deadband.

## Consequences
- Imbues flare deployment with visceral tactile agency, making lighting a beacon in pitch darkness deeply rewarding and physically satisfying.
- Distinct relative stroke mechanics prevent false positives during two-handed firearm stabilization or shiv sharpening.
