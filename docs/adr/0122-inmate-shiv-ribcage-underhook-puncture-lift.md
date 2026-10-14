# ADR 0122: Inmate Shiv Rib-Cage Underhook Puncture & Lift

## Status
Accepted

## Context
In Butcher Bay's close-quarters prison brawls, heavily built inmates frequently overpower straightforward frontal strikes. To counter aggressive charging opponents from a defensive crouch, Riddick can drive a shiv upward into the target's lower thoracic cavity beneath the floating ribs, levering the opponent's torso upward and backward onto the concrete floor.

## Decision
1. **Low Underhook Engagement Stance**:
   - Player physically crouched or leaning low ($Y_{HMD} < -0.25\text{ m} \lor Y_{local, right} < -0.15\text{ m}$).
   - Dominant hand blade gripped in reverse orientation ($Grip_{right} > 0.55 \land Trigger_{right} < 0.40$).
2. **Explosive Upward Diaphragm Thrust & Lift**:
   - Upward/forward rising arm velocity:
     - $V_{y, right} > 1.15\text{ m/s} \land V_{z, right} < -0.90\text{ m/s}$.
3. **Cartilage Shearing & Body Lift Haptics**:
   - Dominant hand: Initial rib cartilage perforation snap ($280\text{ Hz}$, $0.95$, $60\text{ ms}$) transitioning into heavy body hoist tension rumble ($85\text{ Hz}$, $0.70$, $120\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches lethal underhook takedown combination (`W` + `Space` + `LMB`) with a $1500\text{ ms}$ lockout.

## Consequences
- Delivers an explosive low-to-high lethal melee counter-attack that diversifies melee combat alongside carotid slashes (ADR 0099) and eye-gouges (ADR 0092).
- High upward velocity requirement prevents accidental activation during simple standing or locomotion.
