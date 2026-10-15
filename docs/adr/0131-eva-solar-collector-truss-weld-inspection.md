# ADR 0131: Dark Athena EVA Solar Collector Truss Structural Weld Inspection

## Status
Accepted

## Context
During spacewalk excursions across the exterior superstructure of the Dark Athena, micro-meteoroid collisions cause hairline structural fatigue in exterior truss girders. Safety bulkheads remain sealed until all truss frame joints along the transit path are scanned and cleared. Riddick runs his gauntlet scanner smoothly along the structural weld seams to clear clearance locks.

## Decision
1. **Truss Seam Proximity & Alignment**:
   - Controller positioned in seam detection corridor ($Z \in [-0.38, -0.12]\text{ m}, Y \in [-0.15, 0.20]\text{ m}$).
   - Grip held firmly ($Grip > 0.50$).
2. **Continuous Linear Diagnostic Trace**:
   - Controller glides laterally at controlled diagnostic speed ($|V_x| \in [0.25, 0.65]\text{ m/s}$).
   - Motion maintained continuously for $\ge 350\text{ ms}$.
3. **Ultrasonic Scan & Integrity Clearance Haptics**:
   - Continuous high-frequency scanning hum ($230\text{ Hz}$, $0.45$, $350\text{ ms}$).
   - Concluding in a clean structural clearance chime ($180\text{ Hz}$, $0.75$, $90\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches truss inspection clearance sequence (`E` + `Space` + `Tab`) with a $1500\text{ ms}$ lockout.

## Consequences
- Adds an authentic, contemplative EVA spacewalk diagnostic interaction that complements high-intensity exterior hazards.
- Uses controlled velocity windows rather than high-acceleration thrusts to encourage smooth physical motion.
