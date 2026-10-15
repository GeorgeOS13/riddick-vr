# ADR 0361: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Sternum Crushing Ground Pin

## Status
Accepted

## Context
During high-intensity pit brawl encounters against heavy enforcers in Butcher Bay, when an opponent attempts to clinch against structural steel cage columns, Riddick locks a deep whizzer overhook while establishing an iron collar tie on their neck. Pivoting the opponent's thoracic spine across the corner vertex of the structural column, Riddick drives downward violently with rotational reverse-lever momentum, crushing their sternum against the steel flange and pinning them immovably to the deck.

## Decision
Implement physical two-handed spatial gesture detection for the whizzer cranial post sternum crushing ground pin in `src/hook/camera_hook.cpp`:
1. **Close Clinch Range**: Distance between controllers $< 0.26\text{ m}$.
2. **Deep Whizzer & Collar Tie**: Left controller within collar level ($leftLocal.y \in [-0.35, 0.05]\text{ m}$), both grips firmly clenched ($> 0.65$).
3. **Rotational Reverse-Lever Plunge**: Violent downward plunge with rotational reverse-lever momentum ($rVel.y < -2.80\text{ m/s} \land lVel.y < -2.55\text{ m/s} \land |lVel.x| > 1.40\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'W'` + (`'A'`/`'D'`) + `'C'` + `'Space'` + `LMB` + `'Space'` + `'C'` + `LMB` + `'Space'` + `'Q'` + `'Tab'` + `'Return'` + `'Space'` + `'C'` + `'Space'` + `'Q'` + `LMB` (`0x57`, `0x41`/`0x44`, `0x43`, `0x20`, `LMB`, `0x20`, `0x43`, `LMB`, `0x20`, `0x51`, `0x09`, `0x0D`, `0x20`, `0x43`, `0x20`, `0x51`, `LMB`).
5. **Haptic Feedback**: $520\text{ Hz}$ ($0.98$, $260\text{ ms}$) sternum fracture and structural post shockwave on right controller, $450\text{ Hz}$ ($0.95$, $270\text{ ms}$) whizzer overhook drag torque on left controller.

## Consequences
- Delivers an authentic martial-arts submission counter leveraging arena geometry.
- Preserves single-level compound nesting in MSVC compilation.
