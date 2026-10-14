# ADR 0075: Interactive Inmate Gambling Card Flipping & Dealing

## Status
Accepted

## Context
In Butcher Bay recreation areas and inmate pits, prisoners gamble smokes and contraband tokens over cards and dice. While dice mechanics were codified in ADR 0045, cards, cigarette trade cards, and bounty cards require realistic physical card manipulation: holding a hand of cards, fanning them, and flicking them across table surfaces.

## Decision
We implement physical card flipping and dealing gestures:
1. **Spatial Engagement:**
   - Dominant controller positioned above table height ($Y_{local} \in [-0.45, -0.10]\text{ m}, Z_{local} \in [-0.40, -0.15]\text{ m}$).
   - Pinch grip held ($Grip \in [0.40, 0.75]$ with trigger released).
2. **Wrist Flick / Card Deal Velocity:**
   - Rapid lateral wrist flick ($|V_x| > 0.60\text{ m/s} \lor V_z < -0.65\text{ m/s}$).
   - Re-trigger buffer: $300\text{ ms}$.
3. **Engine Action Dispatch & Paper Friction Micro-Haptics:**
   - Dispatches interaction key (`E` / `LMB`) to play the active card.
   - Generates crisp paper card friction micro-haptics ($0.35$ amplitude at $240\text{ Hz}$ for $80\text{ ms}$) simulating card release and felt table glide.

## Consequences
- Heightens the tactile fidelity of prison bartering and recreational gambling mini-games.
- Velocity filtering prevents accidental card plays when resting hands on surfaces.
