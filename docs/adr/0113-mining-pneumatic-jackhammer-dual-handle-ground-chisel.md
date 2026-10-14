# ADR 0113: Heavy Mining Pneumatic Jackhammer Dual-Handle Ground Chisel

## Status
Accepted

## Context
Deep excavation zones in Butcher Bay feature heavy pneumatic bedrock jackhammers with dual shock-absorbing T-bar handles. Escaping through collapsed mines requires Riddick to seize both handles, elevate the chisel bit above fractured rock, and drive downward to shatter stone obstructions.

## Decision
1. **Dual T-Bar Handle Stance**:
   - Both hands spaced horizontally ($dist \in [0.22, 0.45]\text{ m}, Y_{local} \in [-0.45, -0.05]\text{ m}$).
   - Both grips clenched tightly ($Grip_{left} > 0.60 \land Grip_{right} > 0.60$).
2. **Kinetic Downward Ground Chisel Drive**:
   - Both hands plunged downward simultaneously into the deck plane:
     - $V_{y, left} < -1.15\text{ m/s} \land V_{y, right} < -1.15\text{ m/s}$.
3. **Seismic Percussive Ground Fracture Haptics**:
   - Dual-hand violent percussive hammer strike ($50\text{ Hz}$, $0.98$, $110\text{ ms}$) followed by decaying subterranean reverberation ($70\text{ Hz}$, $0.50$).
4. **Engine Input Injection**:
   - Dispatches heavy excavator impact sequence (`S` + `Space` + `LMB`) with a $2000\text{ ms}$ lockout.

## Consequences
- Delivers immense physical feedback when clearing subterranean rubble and opening hidden escape tunnels.
- Coordinated downward velocity requirements prevent interference with regular two-handed weapon handling.
