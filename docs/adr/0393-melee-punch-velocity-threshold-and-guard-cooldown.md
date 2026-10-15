# ADR 0393: Melee Punch Velocity Threshold Lowering & Guard Cooldown Synchronization

## Context
Physical melee combat strikes (straight jabs, blade thrusts, hooks, and uppercuts) previously resided under a high composite velocity gate requiring hand speed exceeding $1.80\text{ m/s}$ ($> 6.5\text{ km/h}$).
However, natural, controlled human punch velocities in room-scale VR typically range between $1.10\text{ m/s}$ and $1.50\text{ m/s}$. Because the threshold was set to $1.80\text{ m/s}$, ordinary jabs and slashes were completely discarded, requiring unnatural, high-effort swings to register.
Furthermore, when throwing a punch from boxer guard, `s_lastPunchTime` was not updated at gesture termination, allowing the guard pose detection to re-engage right-click blocking before the strike could register in the engine.

## Decision
1. Lower the melee attack velocity gate from $1.80\text{ m/s}$ down to $1.10\text{ m/s}$, perfectly matching natural room-scale striking dynamics.
2. Synchronize `s_lastPunchTime = now;` at gesture completion (`combat_gesture_done`) to enforce the 250ms guard lockout buffer across all melee punch and blade strike variants.
3. Automatically release active block (`MOUSEEVENTF_RIGHTUP`) whenever hand speed breaks $1.10\text{ m/s}$ so that punches and blade strikes register cleanly without being swallowed by the guard state.

## Consequences
- Melee combat strikes register fluidly and reliably with natural arm motions.
- Eliminates input conflicts between boxer guard blocking and counter-striking.
