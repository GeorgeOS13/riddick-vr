# ADR 0149: Inmate Pit Brawl Collar Tie Clinch & Headbutt Stun

## Status
Accepted

## Context
In the claustrophobic confines of Butcher Bay's pit fights, brawlers often tie up in close-quarters clinches. Riddick excels at violent infighting counters that turn a collar tie into immediate opponent incapacitation. By locking an off-hand collar grip behind the opponent's neck and violently driving his forehead forward, Riddick delivers a concussive skull collision that fractures the opponent's nasal bridge and leaves them completely staggered.

In VR, incorporating headset kinetic forward motion or coordinated hand-clamping with forward head drive creates an unprecedented level of visceral physical violence.

## Decision
We implement **Inmate Pit Brawl Collar Tie Clinch & Headbutt Stun**:
1. **Collar Tie Clinch Grip**:
   - Off-hand positioned at the opponent neck/collar zone ($Z \in [-0.35, -0.12]\text{ m}$, $Y \in [-0.15, 0.15]\text{ m}$, $Grip > 0.65$).
2. **Dynamic Headbutt Kinetic Impulse**:
   - Violent forward head drive tracked by the HMD ($V_{HMD, z} < -0.80\text{ m/s}$) or coordinated dominant fist/forehead inward surge ($V_{dominant, z} < -1.10\text{ m/s}$).
3. **Input Dispatch**:
   - Dispatches `W` (forward surge) + `Space` (clinch break) + `LMB` (primary attack).
4. **Haptic Feedback**:
   - Dual-controller $0.98$ cranial bone collision concussion shockwave at $180\text{ Hz}$.

## Consequences
- Deepens infighting options from the clinch.
- Cooldown of $1500\text{ ms}$ ensures tactical usage rather than erratic head bobbing.
