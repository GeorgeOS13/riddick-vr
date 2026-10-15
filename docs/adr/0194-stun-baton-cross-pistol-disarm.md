# ADR 0194: Security Guard Stun Baton Forward Two-Handed Cross-Pistol Disarm & Neck Strike

## Status
Accepted

## Context
When facing guards wielding sidearms or short carbines, crossing the stun baton over the enemy's firearm barrel wedges the gun downward while the forward discharge arc electrifies their hands, followed by a sudden forward baton thrust into their throat.

## Decision
Implement physical two-handed firearm trap, disarm arc, and throat thrust in `src/hook/camera_hook.cpp`:
- **Stance & Firearm Trap**: In two-handed stance ($dist < 0.24\text{ m}$) with baton energized ($Grip > 0.60, Trigger > 0.65$), drive both hands downward-forward ($V_y < -0.90\text{ m/s} \land V_z < -0.80\text{ m/s}$) into the enemy weapon corridor ($Y \in [-0.20, 0.10]\text{ m}$).
- **Throat Thrust Follow-Through**: Follow within $500\text{ ms}$ with a violent linear throat thrust ($rVel.z < -1.20\text{ m/s}$).
- **Synthesized Controls**: Emits `W` + `RMB` + `E` + `LMB` for guard parry, weapon strip, and lethal cervical strike.
- **Haptic Feedback**: $280\text{ Hz}$ ($0.95$) electrostatic disarm arc snap, followed by $320\text{ Hz}$ ($0.98$) tracheal kinetic impact.

## Consequences
- Expands anti-firearm tactical responses when fighting armed guards at point-blank range.
- Complements rifle stripping and lateral sweep disarm maneuvers.
