# ADR 0125: Dark Athena EVA Antenna Array Dipole Wave Polarization Alignment

## Status
Accepted

## Context
During spacewalks across the exterior hull of the Dark Athena, jamming fields and pirate signal scramblers obscure long-range distress frequencies and mercenary communication feeds. Riddick must physically reorient the exterior antenna gimbal mast, rotating the dipole array through orthogonal polarization planes to synchronize carrier waves and intercept ship wide tactical orders.

## Decision
1. **Overhead Mast Gimbal Reach**:
   - Both controllers raised to the exterior mast bracket ($Y > 0.16\text{ m}, Z \in [-0.36, -0.06]\text{ m}$).
   - Both grips clenched securely ($Grip_{left} > 0.55 \land Grip_{right} > 0.55$).
2. **Synchronized Azimuth Polarization Sweep**:
   - Both hands swept in tandem through arc adjustments ($|\Delta\vec{V}_{xy}| > 0.45\text{ m/s}$).
   - Requires completing 3 discrete orientation indexing detents.
3. **RF Carrier Resonance & Magnetic Locking Haptics**:
   - Progression haptics: Ascending frequency harmonic drone ($180\text{ Hz} \to 240\text{ Hz}$, $0.35 \to 0.75$) matching signal acquisition.
   - Dual-hand mechanical magnetic index click ($90\text{ Hz}$, $0.85$, $50\text{ ms}$) on successful phase lock.
4. **Engine Input Injection**:
   - Dispatches signal alignment command sequence (`E` + `W` + `Tab`) with a $1500\text{ ms}$ lockout.

## Consequences
- Expands deep space EVA exterior gameplay alongside solar visor handling (ADR 0086), solar bulkhead seals (ADR 0109), solar array hand cranks (ADR 0114), and solar collector shield ejection (ADR 0117).
- Deepens diegetic narrative discovery through physical exterior electronics calibration.
