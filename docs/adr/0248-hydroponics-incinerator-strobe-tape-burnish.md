# ADR 0248: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Phase Angle Stroboscope Target Reflective Tape Burnish

## Status
Accepted

## Context
High-temperature pyrolytic aerosol deposition in the subterranean fan casing reduces optical contrast along the shaft stroboscope reference strip. Burnishing a fresh strip of micro-prismatic reflective tape onto the shaft collar ensures razor-sharp optical pulse triggering and reliable tachometer phase locking.

## Decision
Implement physical stroboscope target reflective tape burnishing mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the reflective target tape alignment applicator against the draft fan shaft (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand firmly presses the roller burnisher tool over the adhesive strip (`twoHandDist < 0.22\text{ m}`, right grip $> 0.55$, right trigger $> 0.50$).
- **Linear Burnishing Stroke**: Deliver a firm forward linear stroke with the right controller (`rVel.z < -0.85\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Enter` to burnish the reflective tape, seat the adhesive substrate, and register optical datum contrast.
- **Haptic Feedback**: $280\text{ Hz}$ ($0.95$) roller burnish seating snap on the right controller and $140\text{ Hz}$ ($0.60$) shaft cylindrical friction on the left controller.

## Consequences
- Guarantees optical trigger reliability under heavy bio-tar aerosol concentrations.
- Seamlessly integrates with stroboscope synchronization, rotor balancing, and fan maintenance.
