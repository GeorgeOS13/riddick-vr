# ADR 0052: Dual-Hand Riot Shield Ballistic Plant & Shield Bash Stance

## Status
Accepted / Implemented

## Context
During prison riots in Butcher Bay (Facility Double-Max, Tower 19) and encounters with mercenary strike teams aboard Dark Athena, armored guards deploy heavy polycarbonate and ballistic steel riot shields. Players acquiring or wielding riot shields require tactical VR interactions beyond standard one-handed aiming:
1. Planting the shield firmly into the floor grating to create stationary barricades under heavy suppression fire.
2. Delivering high-impact physical shield bashes to break enemy guard formations, stumble charging inmates, or deflect incoming projectile barrages.

## Decision
We implement dual-hand ballistic riot shield planting and concussive shield bashing in Section 8/9 of `src/hook/camera_hook.cpp`:
1. **Two-Handed Guard Stance**:
   - Requires two-handed weapon/shield stabilization (`s_twoHandedStabilizationActive`, hand proximity $< 25\text{ cm}$ with off-hand grip held $> 0.40$).
2. **Kinematic Deck Grating Plant**:
   - Plunging both controllers simultaneously downward and slightly forward:
     $V_{y, left} < -0.75\text{ m/s} \land V_{y, right} < -0.75\text{ m/s} \land V_{z, left} < -0.35\text{ m/s} \land V_{z, right} < -0.35\text{ m/s}$.
   - Dispatches athletic crouch lock (`C` + `Space`), anchoring Riddick's defensive stance with a heavy $80\text{ Hz}$ ($0.75$ amplitude) deck clamp impact pulse across both controllers.
3. **Concussive Shield Bash**:
   - Forward explosive two-handed shove from guard stance:
     $V_{z, left} < -1.15\text{ m/s} \land V_{z, right} < -1.15\text{ m/s}$.
   - Dispatches Primary Attack (`LMB` down/up impulse), executing Starbreeze's concussive blunt impact with $140\text{ Hz}$ ($0.95$ amplitude) kinetic shockwave across both hands.

## Consequences
- Transforms defensive riot shield equipment into an active tactical VR weapon.
- Differentiates between defensive downward planting and offensive forward bashing via directional velocity vectors.
- Seamlessly integrates with the existing two-handed SLERP stabilization framework.
