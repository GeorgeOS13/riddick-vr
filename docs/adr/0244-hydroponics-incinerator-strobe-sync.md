# ADR 0244: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Induced Draft Fan Rotor Dynamic Balance Phase Angle Stroboscope Synchronization

## Status
Accepted

## Context
Verifying the dynamic rotor balance of the induced draft fan after clip crimping requires synchronizing a portable optical stroboscope against the spinning shaft keyway. Fine-tuning flash frequency freezes the rotor angle, confirming that unbalance phase vectors have been neutralized.

## Decision
Implement physical stroboscope phase synchronization mechanics in `src/hook/camera_hook.cpp`:
- **Spatial Positioning**: Left hand holds the stroboscopic optical tachometer aimed at the shaft coupling (`leftLocal.z` between $-0.42\text{ m}$ and $-0.16\text{ m}$, `leftLocal.y` between $0.05\text{ m}$ and $0.35\text{ m}$, left grip $> 0.55$). Right hand pinches the flash frequency dial (`twoHandDist < 0.22\text{ m}`, right grip between $0.45$ and $0.85$, right trigger $< 0.35$).
- **Rotary Strobe Tuning**: Rotate the frequency dial with the right controller (`|rVel.x| > 0.65\text{ m/s}`).
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to freeze strobe phase, read balance vectors, and confirm smooth draft induction.
- **Haptic Feedback**: $190\text{ Hz}$ ($0.55$) optical flash sync micro-clicks on the right controller and $310\text{ Hz}$ ($0.90$) stationary rotor strobe lock tone on the left controller.

## Consequences
- Confirms dynamic balance verification for subterranean ventilation blowers.
- Pairs with clip insertion, tang crimping, and sight-glass oil venting.
