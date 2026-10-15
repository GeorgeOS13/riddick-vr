# ADR 0227: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Thermal Blanket Velcro Sealing Flap

## Status
Accepted

## Context
Following mechanical assembly and micrometer tolerance verification, the solar collector articulation hinge must be protected against catastrophic deep-space thermal gradients ($\Delta T > 280\text{ K}$) using a multi-layer insulation (MLI) thermal blanket secured with heavy-duty Velcro and magnetic border strips. Riddick pulls the protective thermal flap across the hinge clevis and seals the seam.

## Decision
Implement physical MLI thermal blanket flap sealing in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Position off-hand holding the insulation flap edge ($Z \in [-0.42, -0.15]\text{ m}, Y \in [0.10, 0.40]\text{ m}$, Left Grip $> 0.55$) and dominant hand pressing the Velcro seam ($dist < 0.22\text{ m}$, Right Grip $> 0.55 \land Right Trigger > 0.50$).
- **Linear Seam Press Stroke**: Sweep dominant hand along the seam ($|rVel.y| > 0.50\text{ m/s} \lor |rVel.x| > 0.50\text{ m/s}$) for $> 250\text{ ms}$.
- **Synthesized Controls**: Emits `E` + `Space` + `Tab` + `Q` for flap alignment, seam tensioning, and magnetic latch adhesion.
- **Haptic Feedback**: $180\text{ Hz}$ ($0.55$) Velcro rasping micro-haptics followed by $260\text{ Hz}$ ($0.90$) magnetic border snap click.

## Consequences
- Finalizes the full EVA solar collector truss articulation thermal and mechanical refurbishment sequence.
- Prepares the solar array for full rotational orientation and power delivery.
