# ADR 0069: Cryo-Chamber Emergency Oxygen Mask Quick-Don Stance

## Status
Accepted

## Context
Across multiple narrative beats in *Assault on Dark Athena* and *Escape from Butcher Bay*, Riddick is exposed to catastrophic environmental decompression, cryo-hibernation pod drainage, and toxic decontamination washouts. In 2D desktop play, surviving environmental asphyxiation or equipping rebreathers was triggered passively by cutscenes or automated zone scripts.

In VR, survivable asphyxiation hazards provide intense immersion when the player must physically reach for and clamp an emergency oxygen mask or rebreather over their face.

## Decision
We implement a physical emergency rebreather / oxygen mask quick-don gesture:
1. **Facial Mask Proximity Envelope:**
   - Off-hand controller brought directly to the mouth/chin zone relative to the HMD ($Z_{local} \in [-0.20, -0.05]\text{ m}, Y_{local} \in [-0.18, +0.06]\text{ m}, |X_{local}| < 0.12\text{ m}$).
   - Off-hand grip held ($Grip > 0.50$) simulating gripping the rebreather cup or regulator harness.
2. **Temporal Hysteresis & Donning Engagement:**
   - Position must be sustained for $\ge 150\text{ ms}$ to eliminate accidental hand passes near the face.
   - De-bounced with a $1500\text{ ms}$ re-trigger buffer.
3. **Engine Action Dispatch & Pneumatic Inhalation Feedback:**
   - Dispatches environmental interaction keystroke (`E`) to seal rebreather valves.
   - Generates an immediate pneumatic intake whoosh on the off-hand controller ($0.65$ amplitude at $110\text{ Hz}$ for $200\text{ ms}$) followed by rhythmic low-frequency regulator breath pulses ($0.35$ at $75\text{ Hz}$).

## Consequences
- Elevates tension in decompression chambers, gas conduits, and EVA airlock cycling sequences.
- Prevents false triggers through tight bounding on face proximity ($|X| < 0.12\text{ m}$, $Z \in [-0.20, -0.05]\text{ m}$).
