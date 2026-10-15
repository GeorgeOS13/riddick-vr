# ADR 0353: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Clavicle Fracture Ground Pin

## Status
Accepted

## Context
In inmate pit brawl combat, when an adversary attempts to maintain a collar tie or head control near structural steel support stanchions, Riddick counters by locking a deep whizzer overhook, driving their cervical spine and shoulder over the stanchion post angle, and dropping all body weight downward across their clavicle into the steel deck frame. This delivers an immediate clavicle fracture, neutralizing their striking arm and pinning them to the deck.

## Decision
Implement physical two-handed spatial gesture detection for the whizzer cranial post clavicle fracture ground pin in `src/hook/camera_hook.cpp`:
1. **Close Clinch Proximity**: Distance between controllers $< 0.26\text{ m}$.
2. **Deep Whizzer & Collar Tie**: Left controller within collar level ($leftLocal.y \in [-0.35, 0.05]\text{ m}$), both grips firmly clenched ($> 0.65$).
3. **Downward Fulcrum Clavicle Slam Stroke**: Violent downward plunge with rotational reverse-lever momentum ($rVel.y < -2.70\text{ m/s} \land lVel.y < -2.45\text{ m/s} \land |lVel.x| > 1.30\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'W'` + (`'A'`/`'D'`) + `'C'` + `'Space'` + `LMB` + `'Space'` + `'C'` + `LMB` + `'Space'` + `'Q'` + `'Tab'` + `'Return'` + `'Space'` + `'C'` + `LMB` (`0x57`, `0x41`/`0x44`, `0x43`, `0x20`, `LMB`, `0x20`, `0x43`, `LMB`, `0x20`, `0x51`, `0x09`, `0x0D`, `0x20`, `0x43`, `LMB`).
5. **Haptic Feedback**: $510\text{ Hz}$ ($0.98$, $250\text{ ms}$) clavicle fracture shockwave on right controller, $440\text{ Hz}$ ($0.95$, $260\text{ ms}$) whizzer overhook drag torque on left controller.

## Consequences
- Provides a devastating clinch submission execution for close-quarters fighting.
- Maintains single compound nesting depth via flattened `goto combat_gesture_done;` architecture.
