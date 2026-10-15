# ADR 0143: Dark Athena EVA Solar Collector Photovoltaic Cell Thermal Decoupling Fuse Pull

## Status
Accepted

## Context
Across the exterior hull of the *Dark Athena*, high-output photovoltaic collector wings channel immense solar energy directly into the ship's primary power grid. During orbital alignment phases near blazing solar flares, overloaded capacitor banks can erupt unless Riddick physically extracts cylindrical high-amperage ceramic thermal decoupling fuses located in recessed hull receptacles.

In native gameplay, this interaction was handled by simple script triggers. In VR, tactile mechanical interaction is heightened by allowing players to physically pinch the ceramic fuse casing and wrench it free from spring-loaded retaining clips.

## Decision
We implement **EVA Solar Collector Thermal Decoupling Fuse Pull**:
1. **Spatial Receptacle Envelope**:
   - Dominant hand inside the recessed fuse receptacle ($Z \in [-0.42, -0.15]\text{ m}$, $Y \in [-0.20, 0.25]\text{ m}$).
   - Pinch grip posture ($Grip \in [0.45, 0.85] \land Trigger < 0.35$).
2. **Extraction Wrench Impulse**:
   - Dynamic rearward/lateral pull stroke ($|\vec{V}| > 0.65\text{ m/s}$) unseats the ceramic cartridge from its spring-loaded retaining clips.
3. **Input Dispatch**:
   - Dispatches `E` (interact) + `Enter` (decouple/fuse pull) + `Q` (power bus isolation).
4. **Haptic Feedback**:
   - Initial $240\text{ Hz}$ ($0.55$) ceramic friction grip followed by an explosive $160\text{ Hz}$ ($0.85$, $100\text{ ms}$) retention detent release snap.

## Consequences
- Deepens Riddick's exterior EVA hull interaction toolset.
- Debounced with a $1500\text{ ms}$ refractory window.
