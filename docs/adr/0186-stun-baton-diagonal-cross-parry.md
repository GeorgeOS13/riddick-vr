# ADR 0186: Security Guard Stun Baton Upward Diagonal Cross-Parry & Counter-Discharge

## Status
Accepted

## Context
When facing overhead melee attacks (pipes, clubs, or heavy shiv strikes), holding the energized stun baton diagonally upward across the chest plane deflects the blow and channels stored capacitor voltage directly into the attacker's weapon arm.

## Decision
Implement physical diagonal stun baton parry and counter-discharge in `src/hook/camera_hook.cpp`:
- **Stance & Readiness**: In two-handed stance ($dist < 0.24\text{ m}$) with stun baton energized ($Grip > 0.60, Trigger > 0.65$).
- **Diagonal High Parry**: Drive both hands diagonally upward across the chest plane ($V_y > 1.15\text{ m/s} \land |V_x| > 0.85\text{ m/s}$, $Y \in [0.05, 0.35]\text{ m}$).
- **Synthesized Controls**: Emits `RMB` + `W` + `Space` + `LMB` for guard parry, weapon bind deflection, and immediate electric counter-shock.
- **Haptic Feedback**: Dual $330\text{ Hz}$ ($0.98$) kinetic deflection detonation shockwave, followed by $160\text{ Hz}$ ($0.75$) conductive arc feedback.

## Consequences
- Protects against heavy overhead melee smashes and unblockable power swings.
- Completes the tactical melee defensive suite alongside the horizontal arc wall and cross-block arc burst.
