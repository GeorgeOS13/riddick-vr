# ADR 0147: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Grease Gun Injection

## Status
Accepted

## Context
During long-duration deep-space transit on the exterior hull of the *Dark Athena*, vacuum cold-welding and solar radiation degrade lubricants in the massive rotary hinge joints of the solar collector wings. In order to manually unfold seized collector trusses and restore emergency ship power, Riddick must position a pneumatic lubricant applicator nozzle against the joint's high-pressure grease nipple and charge the internal bearing raceway.

In VR, this provides an engaging two-handed maintenance interaction requiring steady targeting and trigger depression under low-gravity vacuum conditions.

## Decision
We implement **EVA Solar Collector Truss Articulation Lock Hinge Grease Gun Injection**:
1. **Nozzle Placement Envelope**:
   - Dominant hand positioned at the truss pivot fitting ($Z \in [-0.42, -0.15]\text{ m}$, $Y \in [0.10, 0.40]\text{ m}$).
   - Off-hand stabilizing applicator barrel ($dist(Left, Right) < 0.24\text{ m}$).
2. **Pneumatic Injection Trigger Dwell**:
   - Holding dominant trigger ($> 0.70$) with grip held for $> 400\text{ ms}$ pumps lubricant under pressure into the bearing raceway.
3. **Input Dispatch**:
   - Dispatches `E` (interact) + `Enter` (lubricant injection cycle) + `Tab` (truss deployment status).
4. **Haptic Feedback**:
   - Continuous $160\text{ Hz}$ ($0.50$) fluid displacement buzz during injection, terminating in an assertive $90\text{ Hz}$ ($0.85$, $120\text{ ms}$) hydraulic unseize pop.

## Consequences
- Expands realistic EVA ship-maintenance gestures without cluttering general movement controls.
- Debounced with a $1500\text{ ms}$ refractory cooldown.
