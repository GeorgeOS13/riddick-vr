# ADR 0349: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Ribcage Fracture Deck Slam

## Status
Accepted

## Context
During close-quarters unarmed combat in the Butcher Bay and Dark Athena inmate pit brawl arenas, adversaries frequently initiate aggressive clinch engagements and collar ties. Building upon earlier whizzer throws and post armbar counters (ADR 0341, ADR 0345), Riddick counters enemy clinch pressure by clamping a deep whizzer overhook, controlling the cervical posture, posting the adversary's head against the steel structural post, and dropping all body mass downward across their thoracic cage into the steel grating deck, fracturing ribs and neutralizing the target.

## Decision
Implement physical two-handed spatial gesture detection for the whizzer cranial post fulcrum ribcage fracture deck slam in `src/hook/camera_hook.cpp`:
1. **Close Clinch Proximity**: Distance between controllers $< 0.26\text{ m}$.
2. **Deep Whizzer & Collar Tie**: Left controller within collar level ($leftLocal.y \in [-0.35, 0.05]\text{ m}$), both grips firmly clenched ($> 0.65$).
3. **Downward Fulcrum Slam Stroke**: Violent downward plunge with rotational reverse-lever momentum ($rVel.y < -2.65\text{ m/s} \land lVel.y < -2.40\text{ m/s} \land |lVel.x| > 1.25\text{ m/s}$).
4. **Input Dispatch**: Dispatches key chord `'W'` + (`'A'`/`'D'`) + `'C'` + `'Space'` + `LMB` + `'Space'` + `'C'` + `LMB` + `'Space'` + `'Q'` + `'Tab'` + `'Return'` + `'Space'` + `LMB` (`0x57`, `0x41`/`0x44`, `0x43`, `0x20`, `LMB`, `0x20`, `0x43`, `LMB`, `0x20`, `0x51`, `0x09`, `0x0D`, `0x20`, `LMB`).
5. **Haptic Feedback**: $500\text{ Hz}$ ($0.98$, $250\text{ ms}$) ribcage fracture and structural deck impact shockwave on right controller, $430\text{ Hz}$ ($0.95$, $260\text{ ms}$) whizzer overhook drag torque on left controller.

## Consequences
- Delivers an impactful, fluid martial-arts clinch counter-slam for high-intensity inmate brawl combat.
- Employs flattened `goto combat_gesture_done;` dispatch to maintain fixed compound nesting depth.
