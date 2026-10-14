# ADR 0086: EVA Spacewalk Solar Radiation Shield Visor Downward Pull

## Status
Accepted

## Context
During exterior hull spacewalks on the Dark Athena, direct sunlight in the vacuum of deep space creates blinding solar glare that washes out the HUD and obscures enemy patrol skiffs. In the desktop version, helmet visors were fixed or handled by automated tone-mapping scripts.

In VR, reaching up to the helmet brow and physically pulling down the polarized gold solar visor provides an intensely cinematic and functional sci-fi gesture.

## Decision
We implement a physical solar visor downward pull gesture:
1. **Helmet Brow Proximity Envelope:**
   - Either controller brought to the forehead/brow level ($Y_{local} > 0.18\text{ m}, Z_{local} \in [-0.18, +0.05]\text{ m}, |X_{local}| < 0.20\text{ m}$).
   - Grip held ($Grip > 0.50$) securing the visor rim or pull-tab.
2. **Downward Visor Pull Velocity:**
   - Downward motion of the hand ($V_y < -0.55\text{ m/s}$).
   - Cooldown debounce: $1200\text{ ms}$.
3. **Engine Action Dispatch & Polarized Filter Feedback:**
   - Dispatches native visor filter toggle key (`Q` / `Tab`).
   - Generates a solid mechanical visor latch snap ($0.65$ amplitude at $150\text{ Hz}$ for $120\text{ ms}$) on the active hand.
   - Adjusts optical tone-mapping to cut unshielded solar flare and reveal exterior ship details in high contrast.

## Consequences
- Heightens atmospheric tension and tactile agency during orbital EVA missions.
- Overhead proximity check prevents overlap with chest bandoliers or hip holster draws.
