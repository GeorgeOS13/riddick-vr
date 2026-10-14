# ADR 0093: Subterranean Hydroponics Specimen Harvesting & Pouch Stash

## Status
Accepted

## Context
Subterranean hydroponics facilities deep within Butcher Bay and Dark Athena bio-domes house bioluminescent fungi colonies, medicinal root cultivars, and experimental stimulant herbs. Harvesting these biological specimens replenishes vitality, extends Eyeshine adaptation clarity, and produces valuable bartering commodities among inmates.

In conventional monitor gaming, gathering plants was performed by walking up to hydroponic grow beds and tapping the interaction button (`E`). In VR, gathering subterranean flora requires reaching down to planter trough height, pinching delicate stems between finger and thumb, and executing an upward extraction pull before stashing the specimen in an inventory pouch.

## Decision
1. **Planter Trough Proximity**:
   - Controller positioned at grow-bed elevation ($Y_{rel} \in [-0.55, -0.15]\text{ m}, Z_{rel} \in [-0.42, -0.14]\text{ m}$).
2. **Delicate Pinch Grasp**:
   - Controller grip sensor partially closed ($Grip \in [0.45, 0.80]$) simulating careful finger pinching rather than crushing weapon fist clenches.
3. **Upward Pluck Vector**:
   - Upward extraction velocity ($V_y > 0.40\text{ m/s}$) to detach root tendrils from hydroponic nutrient substrate.
4. **Botanical Snap & Stash Feedback**:
   - Pluck action triggers a crisp $160\text{ Hz}$ ($0.45$) botanical stem snap haptic pulse.
5. **Engine Input Injection**:
   - Injects specimen harvest key sequence (`E`) with an $850\text{ ms}$ debounce deadband.

## Consequences
- Enhances environmental discovery and survival foraging with satisfying physical hand interactions.
- Avoids false triggers during combat crouching by checking partial pinch grip constraints and positive vertical extraction velocity.
