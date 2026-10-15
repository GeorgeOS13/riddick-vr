# ADR 0324: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Correction Trial Weight Lock-Tang Staking Punch

## Status
Accepted

## Context
With the 25-gram steel dynamic balance trial weight clip seated on the designated blade rib, delivering a sharp hammer punch against the clip's integral lock-tang drives its hardened barb into the impeller blade slot, permanently locking the counterweight against centrifugal displacement during high-speed rotation.

## Decision
Implement physical draft fan dynamic balance trial weight lock-tang staking punch mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand stabilizes fan impeller rim (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand aligns staking center punch tool over the balance clip lock tang (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $< 0.40$).
- **Linear Forward Impact Stroke**: Drive right hand forward into the staking tool anvil (`rVel.z < -0.85\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to stake lock tang into blade rib.
- **Haptic Feedback**: $380\text{ Hz}$ ($0.98$) hardened steel center punch detent impact shockwave on dominant controller and $150\text{ Hz}$ ($0.55$) impeller acoustic chime on off-hand controller.

## Consequences
- Permanently secures dynamic balancing counterweights to fan rotor blades.
- Direct prerequisite to subsequent vibration sensor zeroing and draft fan startup.
