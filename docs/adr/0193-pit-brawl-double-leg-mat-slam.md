# ADR 0193: Inmate Pit Brawl Double-Leg Takedown & High-Amplitude Mat Slam

## Status
Accepted

## Context
In full-contact bareknuckle pit brawls, driving under an opponent's high guard allows Riddick to wrap both arms around their thighs, lift them entirely off the deck onto his shoulder, and forcefully spike them into the concrete floor.

## Decision
Implement physical double-leg takedown and high-amplitude slam in `src/hook/camera_hook.cpp`:
- **Level Change & Thigh Wrap**: From low crouch ($Y < -0.30\text{ m}$), drive both hands forward and upward around the thighs ($dist < 0.25\text{ m}$, both grips $> 0.65$, $V_{y, left} > 1.15\text{ m/s} \land V_{y, right} > 1.15\text{ m/s}$).
- **High-Amplitude Spike Slam**: Follow within $500\text{ ms}$ by a downward slam ($V_{y, left} < -1.30\text{ m/s} \land V_{y, right} < -1.30\text{ m/s}$) driving the opponent's torso into the deck grating.
- **Synthesized Controls**: Emits `C` + `W` + `Space` + `S` + `LMB` for level change, lift, spine-jarring deck plant, and mount establishment.
- **Haptic Feedback**: Dual $210\text{ Hz}$ ($0.90$) heavy body lift strain, followed by dual $40\text{ Hz}$ ($0.98$) concussive mat impact blast.

## Consequences
- Delivers an impactful, devastating wrestling throw for aggressive close-quarters encounters.
- Integrates cleanly with single-leg ankle picks, underhook suplexes, and mount ground-and-pound.
