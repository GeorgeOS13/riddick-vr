# ADR 0084: Subterranean Ventilation Axial Fan Blade Jamming Bar

## Status
Accepted

## Context
In *Escape from Butcher Bay* and *Assault on Dark Athena*, massive spinning industrial turbine fans block ventilation shafts. In flat desktop games, disabling these fans usually involved locating a distant control breaker or shooting a power relay.

In VR, physical environmental interaction enables players to thrust an improvised steel crowbar, pipe, or reinforced club directly between the spinning fan blades, physically jamming the rotor and halting the lethal blades with explosive mechanical torque.

## Decision
We implement physical ventilation turbine fan blade jamming:
1. **Spatial Proximity Envelope:**
   - Both hands positioned at the duct fan cowl ($Z_{local} \in [-0.42, -0.16]\text{ m}, Y_{local} \in [-0.15, +0.25]\text{ m}$).
   - Both controller grips engaged ($Grip_{left} > 0.50 \land Grip_{right} > 0.50$) holding a heavy tool or pipe.
2. **Forward Jamming Thrust Kinematics:**
   - Forward thrust velocity exceeding $-1.10\text{ m/s}$ ($V_{z, left} < -1.10\text{ m/s} \lor V_{z, right} < -1.10\text{ m/s}$).
   - Cooldown buffer: $1200\text{ ms}$.
3. **Engine Action Dispatch & Mechanical Jamming Shudder:**
   - Dispatches interaction and power interrupt keystrokes (`E` + `Space`).
   - Generates dual synchronized high-impact mechanical shockwaves ($0.95$ amplitude at $120\text{ Hz}$ for $220\text{ ms}$) simulating the violent arrest of multi-ton spinning steel blades.

## Consequences
- Creates an unforgettable physical environmental puzzle solution.
- Dual-handed requirement and high forward velocity threshold prevent false-triggers during normal duct crawling.
