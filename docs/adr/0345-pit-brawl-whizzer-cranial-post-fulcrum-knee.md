# ADR 0345: Inmate Pit Brawl Double Collar Tie Whizzer Counter-Throw & Thoracic Spine Cranial Post Fulcrum Leverage Sternum Knee Drive

## Status
Accepted

## Context
When maintaining an adversary pinned against a vertical structural post corner, wedging their spine into the corner vertex and launching an explosive upward knee strike directly into their sternum while wrenching their collar tie downward delivers traumatic concussive compression that breaks their guard and ends the fight.

## Decision
Implement physical whizzer cranial post fulcrum leverage sternum knee drive mechanics in `src/hook/camera_hook.cpp`:
- **Clinch Overhook & Pin Stance**: In close brawl range (`handsDistSledge < 0.26\text{ m}`), lock deep whizzer overhook and collar tie (`leftLocal.y` between $-0.35\text{ m}$ and $0.05\text{ m}$, both grips $> 0.65$, right grip $> 0.65$).
- **Violent Fulcrum Sternum Knee Drive Plunge**: Plunge downward and drive opposing lateral torque across post corner (`rVel.y < -2.60\text{ m/s} \land lVel.y < -2.35\text{ m/s} \land |lVel.x| > 1.20\text{ m/s}`).
- **Lateral Vector Resolution**: Direction determined by lateral whip velocity vector (`lVel.x > 0 \to \text{'D'}` (right), else `'A'` (left)).
- **Synthesized Controls**: Emits `W` + Direction (`A`/`D`) + `C` + `Space` + `LMB` + `Space` + `C` + `LMB` + `Space` + `Q` + `Tab` + `Space` + `Enter` + `LMB` for corner pin, sternum knee drive impact, and concussive deck follow-through.
- **Haptic Feedback**: $500\text{ Hz}$ ($0.98$) sternum impact and structural post shockwave on dominant hand, $420\text{ Hz}$ ($0.95$) whizzer collar tie drag torque on off-hand.

## Consequences
- Traumatic close-quarters blunt force finisher.
- Exploits structural vertical post geometry to defeat heavily armored opponents.
