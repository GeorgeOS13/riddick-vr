# ADR 0118: Tactical Stun Baton Overload Arc Wave Discharge

## Status
Accepted

## Context
When surrounded by multiple inmates or riot security squads, single-target stun baton strikes (ADR 0055) and ground defibrillator slams (ADR 0077) may leave vulnerable flanks exposed. By crossing both hands at chest level while squeezing both controller grips and triggers to short-circuit the baton's capacitor bus, Riddick can unleash a radial electrostatic arc wave discharge.

## Decision
1. **Crossed-Arm Capacitor Short-Circuit Stance**:
   - Both hands brought together in an 'X' block at chest height ($dist < 14\text{ cm}, Y_{local} \in [-0.25, 0.10]\text{ m}$).
   - Both grips clenched tightly ($Grip_{left} > 0.70 \land Grip_{right} > 0.70$) and both triggers held ($Trigger_{left} > 0.65 \land Trigger_{right} > 0.65$).
   - Sustained for at least $250\text{ ms}$ to build peak electrostatic charge.
2. **Explosive Outward Radial Arc Thrust**:
   - Both hands thrust outward laterally and forward:
     - $V_{x, left} < -1.05\text{ m/s} \land V_{x, right} > 1.05\text{ m/s}$ (or forward $V_z < -1.10\text{ m/s}$).
3. **High-Voltage Electrostatic Arc Ring Haptics**:
   - Charging phase: Escalating high-voltage ionization whine ($180\text{ Hz} \to 290\text{ Hz}$, $0.30 \to 0.75$).
   - Discharge wave: Dual-hand concussive electrostatic shock ring ($300\text{ Hz}$, $0.98$, $120\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches radial stun wave sequence (`S` + `LMB` + `Space` + `Q`) with a $2500\text{ ms}$ recharge deadband.

## Consequences
- Empowers the player to clear immediate surrounding threats during high-density riot encounters.
- Multi-input charge requirement (crossed hands + grips + triggers + hold time) guarantees no false positives during standard combat.
