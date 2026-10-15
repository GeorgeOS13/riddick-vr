# ADR 0145: Inmate Pit Brawl Liver Hook & Sternum Elbow Smash Combination

## Status
Accepted

## Context
In Butcher Bay's subterranean fighting pit, tight quarters demand brutal close-range combination strikes. Against towering enforcers or armored brutes, a standard cross or jab fails to penetrate heavy body armor or break high guards. Riddick utilizes a devastating infighting combination: driving a vicious rising shovel hook into the vulnerable hepatic zone (liver) under the ribcage to buckle the opponent, followed immediately by bringing the lead elbow down hard onto their exposed sternum.

In VR, physical immersion is elevated when multi-hit close-quarters combinations reward natural kinetic chaining from block stance.

## Decision
We implement **Inmate Pit Brawl Liver Hook & Sternum Elbow Smash Combination**:
1. **Boxer Guard Stance**:
   - Player is actively maintaining Boxer Guard (`s_currentBlockEngaged`).
2. **Kinetic Strike Chaining**:
   - **Step 1 (Liver Hook)**: Driving dominant fist in a tight upward/inward arc ($V_{dominant, y} > 1.10\text{ m/s} \land |V_{dominant, x}| > 0.90\text{ m/s}$). Registers liver strike and starts a $500\text{ ms}$ combo window.
   - **Step 2 (Sternum Elbow)**: Plunging dominant hand/elbow downward ($V_{dominant, y} < -1.20\text{ m/s}$) within $500\text{ ms}$ completes the combo.
3. **Input Dispatch**:
   - Dispatches `W` (forward advance) + `A` (lateral torque) + `LMB` (primary attack).
4. **Haptic Feedback**:
   - Liver hook: $0.95$ soft-tissue concussive shockwave at $220\text{ Hz}$.
   - Sternum elbow: $0.98$ bone-fracturing blunt crack at $280\text{ Hz}$.

## Consequences
- Deepens the dynamic physical combo system in unarmed pit fights.
- Natural extension of the Boxer Guard mechanics, rewarding rhythmic follow-through.
