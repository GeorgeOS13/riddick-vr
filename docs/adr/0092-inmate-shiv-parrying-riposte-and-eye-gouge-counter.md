# ADR 0092: Inmate Shiv Parrying Riposte & Eye-Gouge Counter

## Status
Accepted

## Context
Close-quarters blade combat in Butcher Bay pits Riddick against lethal inmates armed with improvised shivs, Ulak daggers, and razor knuckles. Starbreeze's melee engine features a specialized lethal counter-strike mechanic: initiating an attack while blocking absorbs the assailant's thrust and counter-punches into their throat or ocular orbital sockets.

In VR, executing a deadly counter-riposte should reward precise spatial timing: maintaining high Boxer Guard or blade cross-parry, absorbing an incoming kinetic strike, and immediately launching an upward forward lunge directly into the opponent's eye level.

## Decision
1. **Guard Stance Anchor**:
   - Player must be in active Boxer Guard / blade block (`s_currentBlockEngaged` held for $\ge 60\text{ ms}$).
2. **Upward Ocular Thrust Trajectory**:
   - Dominant controller thrusting rapidly upward and forward:
     - Forward velocity: $V_z < -1.10\text{ m/s}$.
     - Upward velocity: $V_y > 1.05\text{ m/s}$.
     - Elevation at terminus: $Y_{rel} > 0.04\text{ m}$ (eye/orbital cavity level).
3. **Bone & Cartilage Puncture Haptics**:
   - Synthesizes violent $280\text{ Hz}$ ($0.95$) dual-phase cartilage perforation and cranial crunch haptics ($65\text{ ms}$ duration).
4. **Engine Input Injection**:
   - Dispatches forward power attack (`W` + `LMB`) with native Starbreeze disarm/lethal riposte translation.

## Consequences
- Elevates edged-weapon duels into lethal martial art encounters where defensive parries naturally flow into visceral anatomical finishers.
- Clean separation from standard hooks and body jabs via mandatory upward velocity vector ($V_y > 1.05\text{ m/s}$) targeting eye elevation.
