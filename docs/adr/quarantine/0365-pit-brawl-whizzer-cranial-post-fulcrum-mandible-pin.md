# ADR 0365: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Mandible Dislocation Ground Pin

## Status
Accepted

## Context
During close-quarters inmate pit brawls in Butcher Bay, opponents frequently attempt to absorb concussive impacts by pinning their shoulders against reinforced cage stanchions. Riddick capitalizes on this tactical error by establishing a double collar tie, transitioning into a deep whizzer underhook, and driving the opponent's skull against the corner vertex of the structural post. Dropping violently into a downward rotational reverse-lever plunge uses the post edge as a cranial fulcrum to dislocate the adversary's mandible and pin them securely to the deck grating.

## Decision
Implement physical two-handed spatial gesture detection for the whizzer cranial post mandible dislocation ground pin in `src/hook/camera_hook.cpp`:
1. **Close Clinch Range**: Distance between controllers $< 0.26\text{ m}$.
2. **Deep Whizzer & Collar Tie**: Left controller within collar level ($leftLocal.y \in [-0.35, 0.05]\text{ m}$), both grips firmly clenched ($> 0.65$).
3. **Rotational Reverse-Lever Plunge**: Violent downward plunge with rotational reverse-lever momentum ($rVel.y < -2.85\text{ m/s} \land lVel.y < -2.60\text{ m/s} \land |lVel.x| > 1.45\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'W'` + (`'A'`/`'D'`) + `'C'` + `'Space'` + `LMB` + `'Space'` + `'C'` + `LMB` + `'Space'` + `'Q'` + `'Tab'` + `'Return'` + `'Space'` + `'C'` + `'Space'` + `'Q'` + `'Return'` + `LMB` (`0x57`, `0x41`/`0x44`, `0x43`, `0x20`, `LMB`, `0x20`, `0x43`, `LMB`, `0x20`, `0x51`, `0x09`, `0x0D`, `0x20`, `0x43`, `0x20`, `0x51`, `0x0D`, `LMB`).
5. **Haptic Feedback**: $530\text{ Hz}$ ($0.98$, $270\text{ ms}$) mandible fracture and post impact shockwave on right controller, $460\text{ Hz}$ ($0.95$, $280\text{ ms}$) whizzer overhook drag torque on left controller.

## Consequences
- Delivers an authentic martial-arts submission counter leveraging arena geometry.
- Preserves single-level compound nesting in MSVC compilation.
