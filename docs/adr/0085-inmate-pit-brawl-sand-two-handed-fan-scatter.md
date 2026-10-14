# ADR 0085: Inmate Pit Brawl Sand / Ash Throw Two-Handed Fan Scatter

## Status
Accepted

## Context
In Butcher Bay and Dark Athena arena brawls, multiple aggressive inmates frequently surround Riddick simultaneously. While single-handed dirt throws (ADR 0065) target an individual attacker directly ahead, surviving multi-adversary ambushes requires a broad, sweeping crowd-control dispersion.

## Decision
We implement a two-handed wide-arc sand/ash fan scatter gesture:
1. **Low Crouched Stance & Proximity:**
   - Player must be physically crouched ($Y_{drop} > 0.25\text{ m}$).
   - Both hands positioned low near the dirt/sand deck ($Y_{local, left} < -0.40\text{ m} \land Y_{local, right} < -0.40\text{ m}$).
   - Hands unclasped ($Grip < 0.45 \land Trigger < 0.40$) scooping dual handfuls of particulate soot.
2. **Outward Lateral Arm Fanning Velocity:**
   - Rapid opposing outward arm swings: left hand moves left ($V_{x, left} < -0.80\text{ m/s}$), right hand moves right ($V_{x, right} > +0.80\text{ m/s}$).
   - Simultaneously rising slightly ($V_{y} > +0.50\text{ m/s}$).
   - Cooldown buffer: $1400\text{ ms}$.
3. **Engine Action Dispatch & Wide Dispersion Tactile Feedback:**
   - Dispatches interaction and wide-sweep melee attack (`E` + `LMB`).
   - Generates dual synchronized wide-spectrum particulate scatter haptic bursts ($0.65$ amplitude at $180\text{ Hz}$ for $180\text{ ms}$) blinding all frontal assailants across a 180-degree field of view.

## Consequences
- Equips the player with an authentic, physical area-of-effect crowd-control mechanic.
- Strict crouching and opposing lateral velocity gating prevents accidental firing during standard standing punches.
