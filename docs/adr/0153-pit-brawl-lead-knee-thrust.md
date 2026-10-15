# ADR 0153: Inmate Pit Brawl Muay Thai Lead Knee Thrust to Abdomen

## Status
Accepted

## Context
In the claustrophobic melee combat of the inmate fighting pits, when an opponent closes distance or attempts a grappling clinch, high punches can be smothered. Riddick counters with a devastating, explosive lead knee strike directly into the opponent's solar plexus, liver, or diaphragm, fracturing their lower ribs and violently driving the air from their lungs.

In VR, coordinating an upward dominant strike while framing or pulling down with the off-hand naturally captures the authentic kinetic mechanics of a Muay Thai lead knee strike.

## Decision
We implement **Inmate Pit Brawl Muay Thai Lead Knee Thrust to Abdomen**:
1. **Guard Pre-requisite**:
   - Player is in active Boxer Guard (`s_currentBlockEngaged`).
2. **Opposing Kinetic Surge**:
   - Driving the dominant hand upward and forward ($V_{dominant, y} > 1.20\text{ m/s}$) while plunging the off-hand downward to trap/frame ($V_{offhand, y} < -0.85\text{ m/s}$).
3. **Input Dispatch**:
   - Dispatches `W` (forward surge) + `C` (low-line strike) + `LMB` (primary attack).
4. **Haptic Feedback**:
   - Dominant hand delivers a bone-crushing $0.98$ concussive organ shockwave at $200\text{ Hz}$, while the framing off-hand receives a $0.65$ body clamp vibration at $100\text{ Hz}$.

## Consequences
- Powerful close-range counter against advancing brawlers.
- $1500\text{ ms}$ cooldown prevents erratic spamming.
