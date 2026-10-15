# ADR 0141: Inmate Pit Brawl Shin Kick & Knee Stomp Hyper-Extension

## Status
Accepted

## Context
In the brutal, no-rules inmate pit fighting arenas of Butcher Bay, opponent brawlers often charge with reckless forward momentum. Rather than relying solely on high punches and forearm parries, Riddick can exploit low vulnerabilities by driving a heel or foot down an opponent's shin into the knee joint to cause hyperextension, completely collapsing their guard and driving them into the floor.

In VR, physical immersion requires giving players intuitive, bone-crushing low-line striking options while maintaining defensive stability or crouching.

## Decision
We implement **Inmate Pit Brawl Shin Kick & Knee Stomp Hyper-Extension**:
1. **Defensive Posture Pre-requisite**:
   - Player is actively maintaining Boxer Guard (`s_currentBlockEngaged`) or crouched in low fighting stance (`s_isPhysicallyCrouched || s_isDeepProne`).
2. **Kinetic Foot Plant & Stomp Strike**:
   - Driving the dominant hand violently downward ($V_{dominant, y} < -1.10\text{ m/s}$) while pushing the off-hand forward to pin or frame ($V_{offhand, z} < -0.70\text{ m/s}$).
3. **Input Dispatch**:
   - Dispatches `C` (crouch/low-line) + `S` (ground plant) + `LMB` (primary attack).
4. **Haptic Feedback**:
   - Dominant hand delivers a brutal $0.98$ concussive skeletal fracture shockwave at $240\text{ Hz}$, while the off-hand receives a $0.60$ bracing pulse at $100\text{ Hz}$.

## Consequences
- Expands Riddick's unarmed close-quarters combat repertoire with devastating low-line counters.
- Cooldown of $1500\text{ ms}$ prevents repetitive spam and rewards tactical timing against charging enemies.
