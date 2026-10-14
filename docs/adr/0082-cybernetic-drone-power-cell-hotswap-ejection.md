# ADR 0082: Cybernetic Drone HUD Battery Power Cell Hot-Swap Ejection

## Status
Accepted

## Context
In *Assault on Dark Athena*, piloting ghost security drones or firing heavy military railguns consumes high-draw electrical capacitors. In standard desktop play, weapon reloads used a simple press of the 'R' key.

In VR, physical gunplay immersion reaches new heights when players can manually rip or flick depleted cylindrical battery power cells from weapon/drone receivers before slapping fresh cells home.

## Decision
We implement a physical power cell hot-swap ejection gesture:
1. **Receiver Proximity Envelope:**
   - Off-hand controller brought close to dominant hand / firearm receiver ($dist_{hands} < 0.18\text{ m}$).
   - Off-hand grip held ($Grip > 0.50$) gripping the thermal battery cell sleeve.
2. **Downward Hot-Swap Ejection Flick:**
   - Downward vertical flick of the off-hand ($V_{y, left} < -0.70\text{ m/s}$).
   - Cooldown debounce: $1100\text{ ms}$.
3. **Engine Action Dispatch & Magnetic Disconnect Shock:**
   - Dispatches weapon reload key (`R`).
   - Generates an immediate magnetic coupling disconnect snap on the off-hand ($0.75$ amplitude at $140\text{ Hz}$ for $120\text{ ms}$) followed by dominant controller slide chambering pulse ($0.50$ at $200\text{ Hz}$).

## Consequences
- Creates a visceral tactile reload alternative for high-tech energy weapons and cybernetic drones.
- Clear downward velocity check ($V_y < -0.70\text{ m/s}$) differentiates this from steady two-handed foregrip holding (ADR 0005).
