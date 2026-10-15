# ADR 0357: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Scapula Fracture Deck Pin

## Status
Accepted

## Context
During high-intensity pit brawl encounters in Butcher Bay, opponents frequently seek to recover from failed strikes by clinching against structural steel support columns. Riddick exploits their posture by locking a deep whizzer overhook, controlling the neck with a collar tie, and hauling the opponent's upper torso backward over the structural corner edge. Dropping downward violently with rotational reverse-lever momentum crushes the adversary's scapula against the steel post angle, pinning them to the deck.

## Decision
Implement physical two-handed spatial gesture detection for the whizzer cranial post scapula fracture deck pin in `src/hook/camera_hook.cpp`:
1. **Close Clinch Range**: Distance between controllers $< 0.26\text{ m}$.
2. **Deep Whizzer & Collar Tie**: Left controller within collar level ($leftLocal.y \in [-0.35, 0.05]\text{ m}$), both grips firmly clenched ($> 0.65$).
3. **Rotational Reverse-Lever Plunge**: Violent downward plunge with rotational reverse-lever momentum ($rVel.y < -2.75\text{ m/s} \land lVel.y < -2.50\text{ m/s} \land |lVel.x| > 1.35\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'W'` + (`'A'`/`'D'`) + `'C'` + `'Space'` + `LMB` + `'Space'` + `'C'` + `LMB` + `'Space'` + `'Q'` + `'Tab'` + `'Return'` + `'Space'` + `'C'` + `'Space'` + `LMB` (`0x57`, `0x41`/`0x44`, `0x43`, `0x20`, `LMB`, `0x20`, `0x43`, `LMB`, `0x20`, `0x51`, `0x09`, `0x0D`, `0x20`, `0x43`, `0x20`, `LMB`).
5. **Haptic Feedback**: $510\text{ Hz}$ ($0.98$, $250\text{ ms}$) scapula fracture and structural post shockwave on right controller, $440\text{ Hz}$ ($0.95$, $260\text{ ms}$) whizzer overhook drag torque on left controller.

## Consequences
- Delivers an authentic martial-arts submission counter leveraging arena geometry.
- Preserves single-level compound nesting in MSVC compilation.
