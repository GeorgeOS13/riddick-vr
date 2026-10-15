# ADR 0369: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Zygomatic Arch Fracture Ground Pin

## Status
Accepted

## Context
During high-intensity pit brawl encounters in Butcher Bay, aggressive opponents frequently drive forward attempting to slam Riddick into arena perimeter steel beams. Riddick reverses their forward drive by locking a collar tie and wrapping an overhook into a deep whizzer clinch. Leveraging the sharp 90-degree corner vertex of the structural post against the opponent's zygomatic arch (cheekbone), Riddick plunges downward violently with rotational reverse-lever momentum, fracturing the facial bone structure and pinning the stunned combatant securely to the arena floor grating.

## Decision
Implement physical two-handed spatial gesture detection for the whizzer cranial post zygomatic arch fracture ground pin in `src/hook/camera_hook.cpp`:
1. **Close Clinch Range**: Distance between controllers $< 0.26\text{ m}$.
2. **Deep Whizzer & Collar Tie**: Left controller within collar level ($leftLocal.y \in [-0.35, 0.05]\text{ m}$), both grips firmly clenched ($> 0.65$).
3. **Rotational Reverse-Lever Plunge**: Violent downward plunge with rotational reverse-lever momentum ($rVel.y < -2.90\text{ m/s} \land lVel.y < -2.65\text{ m/s} \land |lVel.x| > 1.50\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'W'` + (`'A'`/`'D'`) + `'C'` + `'Space'` + `LMB` + `'Space'` + `'C'` + `LMB` + `'Space'` + `'Q'` + `'Tab'` + `'Return'` + `'Space'` + `'C'` + `'Space'` + `'Q'` + `'Return'` + `'Space'` + `LMB` (`0x57`, `0x41`/`0x44`, `0x43`, `0x20`, `LMB`, `0x20`, `0x43`, `LMB`, `0x20`, `0x51`, `0x09`, `0x0D`, `0x20`, `0x43`, `0x20`, `0x51`, `0x0D`, `0x20`, `LMB`).
5. **Haptic Feedback**: $540\text{ Hz}$ ($0.98$, $280\text{ ms}$) zygomatic bone fracture and structural post shockwave on right controller, $470\text{ Hz}$ ($0.95$, $290\text{ ms}$) whizzer overhook drag torque on left controller.

## Consequences
- Delivers an authentic martial-arts submission counter leveraging arena geometry.
- Preserves single-level compound nesting in MSVC compilation.
