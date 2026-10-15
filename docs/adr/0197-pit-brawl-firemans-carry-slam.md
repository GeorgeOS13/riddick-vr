# ADR 0197: Inmate Pit Brawl Fireman's Carry & Deck Body-Spike Slam

## Status
Accepted

## Context
Against aggressive adversaries rushing with wild overhands, ducking beneath their punch allows Riddick to drive an underhook, pull their arm across his shoulders into a Fireman's carry, hoist them off their feet, and violently whip them down onto the concrete deck.

## Decision
Implement physical Fireman's carry throw and deck body-spike slam in `src/hook/camera_hook.cpp`:
- **Duck-Under & Shoulder Load**: From active Boxer Guard or low crouch ($Y < -0.25\text{ m}$), drive dominant arm between legs / underhook ($rVel.y > 1.10\text{ m/s}, lVel.z > 0.70\text{ m/s}$, both grips $> 0.65$).
- **Rotational Whip Throw**: Follow within $500\text{ ms}$ by a violent lateral throw across the torso ($|V_{x, left}| > 1.25\text{ m/s} \lor |V_{x, right}| > 1.25\text{ m/s}$) spiking the opponent onto the floor.
- **Synthesized Controls**: Emits `W` + `A`/`D` (throw direction) + `Space` + `C` + `LMB` for level change, load, rotational pitch throw, and impact shock.
- **Haptic Feedback**: $220\text{ Hz}$ ($0.90$) shoulder carry strain, followed by dual $45\text{ Hz}$ ($0.98$) deck body-spike shockwave.

## Consequences
- Adds an athletic, dynamic throw to Riddick's unarmed close-quarters moveset.
- Seamlessly transitions from duck-under counters and clinches into ground control.
