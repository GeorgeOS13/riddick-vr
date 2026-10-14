# ADR 0115: Inmate Pit Brawl Pocket Ash Throw Riposte

## Status
Accepted

## Context
In Butcher Bay arena fights, inmates who have gathered and concealed abrasive grit or furnace ash in their waist pockets (ADR 0096) can turn defensive blocks into lethal counter-opportunities. While holding Boxer Guard, dipping the hand down to touch the waist band and immediately flinging the handful upward into an oncoming opponent's eyes creates an instant blind stun window followed by an unblockable riposte.

## Decision
1. **Pocket Grit Draw from Guard**:
   - Active Boxer Guard engaged (`s_currentBlockEngaged`).
   - Dominant hand dips down to waist pocket corridor ($Y_{local} \in [-0.45, -0.15]\text{ m}, |X_{local}| \in [0.10, 0.26]\text{ m}$) with grip held ($Grip > 0.50$).
2. **Explosive Upward / Forward Dust Fling**:
   - Dominant hand launched upward and forward:
     - $V_y > 1.10\text{ m/s} \land V_z < -0.90\text{ m/s}$.
3. **Dual Particulate & Strike Haptics**:
   - Initial cloth rustle micro-tap ($180\text{ Hz}$, $0.40$).
   - Concussive particulate scatter & strike snap ($220\text{ Hz}$, $0.85$) upon extension.
4. **Engine Input Injection**:
   - Injects blind and counter-punch combination (`E` + `Tab` + `W` + `LMB`) with a $1600\text{ ms}$ re-gather deadband.

## Consequences
- Merges dirty arena pocket inventory tactics with active Boxer Guard defensive flow.
- Fast vertical velocity threshold ensures no misfires during standard blocks or hooks.
