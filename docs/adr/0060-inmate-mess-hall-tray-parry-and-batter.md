# ADR 0060: Inmate Mess Hall Tray Blunt Defense & Batter

## Status
Accepted / Implemented

## Context
During prison mess hall riots and surprise shiv ambushes in Butcher Bay's recreation facilities, stamped metal food trays are the most ubiquitous defensive objects. Inmates scoop trays from tables to block incoming blades and smash opponents in the face, creating concussive staggers.

In VR, grabbing a cafeteria tray with the off-hand to deflect incoming attacks, followed by a violent forward smash, captures the raw, visceral reality of prison survival.

## Decision
We implement metal food tray parrying and concussive facial battering in Section 5a/6 of `src/hook/camera_hook.cpp`:
1. **Table Surface Reach & Tray Equip**:
   - Controller positioned at table height:
     $Y_{local} \in [-0.55, -0.25]\text{ m}$, $Z_{local} \in [-0.40, -0.15]\text{ m}$.
   - Gripping with Left hand ($> 0.50$) equips improvised metal mess tray (`E` + Slot '1') with $180\text{ Hz}$ ($0.40$) sheet metal clang.
2. **Metallic Blade Parry**:
   - Holding tray forward while an enemy attacks deflects kinetic strikes:
     Emits loud $200\text{ Hz}$ ($0.80$ amplitude) metal deflection clatter.
3. **Concussive Facial Batter**:
   - Forward explosive horizontal tray thrust ($V_{z, left} < -1.0\text{ m/s}$):
   - Dispatches Primary Attack (`LMB` down/up), staggering incoming inmates with $160\text{ Hz}$ ($0.90$ amplitude) concussive blunt impact.

## Consequences
- Expands improvised melee combat in early-game prison environments.
- Distinguishes off-hand metal tray defense from bare-fist Boxer Guard.
- Delivers visceral auditory and tactile sheet-metal feedback.
