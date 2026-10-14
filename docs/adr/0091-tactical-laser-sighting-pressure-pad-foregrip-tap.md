# ADR 0091: Tactical Laser Sighting Pressure Pad Foregrip Tap

## Status
Accepted

## Context
Advanced military rifles and custom mercenary sidearms across Butcher Bay and Dark Athena feature rail-mounted optical laser sighting modules. In flat screen gaming, toggling tactical accessories was assigned to auxiliary keyboard bindings (such as `F` or `L`).

In virtual reality, holding a firearm in a stabilized two-handed shooting stance places the operator's support hand directly onto the weapon's forend. On modern combat firearms, tactical weapon lights and aiming lasers are actuated via pressure tape switches mounted along the handguard. Replicating this tactile manipulation in VR provides seamless tactical versatility without disrupting aim alignment.

## Decision
1. **Two-Handed Stance Prerequisite**:
   - Operator must be in active two-handed stabilization ($dist < 25\text{ cm}$ with off-hand grip held $> 0.45$).
2. **Pressure Pad Actuation**:
   - Squeezing off-hand index trigger lightly ($Trigger_{left} > 0.50$) or clicking off-hand thumbstick toggles the tactical laser diode circuit.
3. **Optoelectronic Micro-Haptics**:
   - Synthesizes a crisp $180\text{ Hz}$ ($0.45$) tactile micro-switch click in the support hand upon engagement, and a softer $120\text{ Hz}$ ($0.25$) release pulse.
4. **Engine Input Injection**:
   - Injects tactical light / accessory toggle key (`F`) with a $350\text{ ms}$ debounce to enable rapid deliberate toggles during CQB corridor clearing.

## Consequences
- Eliminates the need to remove hands from the weapon to access weapon-mounted aiming aids.
- Provides immediate physical feedback matching real-world rail-mounted pressure tape switches.
