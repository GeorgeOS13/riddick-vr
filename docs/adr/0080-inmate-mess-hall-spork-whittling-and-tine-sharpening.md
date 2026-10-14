# ADR 0080: Inmate Mess Hall Contraband Spork Whittling & Tine Sharpening

## Status
Accepted

## Context
In Butcher Bay cafeteria and recreation sequences, inmates conceal stamped mess utensils, such as plastic and aluminum sporks, to craft improvised puncturing shivs. While toothbrush shanks are ground to a singular chisel tip (ADR 0068), meal sporks feature tines that are notched and honed against table rivets to create triple-fanged puncture tools.

## Decision
We implement physical spork tine whittling and hone grinding:
1. **Low Table / Bench Contact Envelope:**
   - Dominant controller positioned at seated/table height ($Y_{local} \in [-0.55, -0.25]\text{ m}, Z_{local} \in [-0.42, -0.16]\text{ m}$).
   - Player physically crouched or leaning forward ($Y_{drop} > 0.20\text{ m}$).
   - Grip held ($Grip > 0.50$).
2. **Notching & Grinding Kinematics:**
   - Fast reciprocating strokes ($|V_x| > 0.60\text{ m/s}$).
   - Requires completing 4 stroke passes ($now - s\_lastSporkStrokeTime > 160\text{ ms}$).
3. **Engine Action Dispatch & Serrated Metallic Rasp:**
   - On completion of 4 strokes, triggers item craft/equip (`E` + Slot '3').
   - Emits triple-frequency serrated rasp pulses on each stroke ($0.45$ amplitude at $190\text{ Hz}$), finishing with a resonant hone ping ($0.70$ at $240\text{ Hz}$).

## Consequences
- Expands the variety of improvised prison crafting mechanics available during stealth and dialogue downtime.
- Clean height and stance gating prevents accidental triggering during upright exploration.
