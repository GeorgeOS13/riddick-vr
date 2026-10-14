# ADR 0099: Inmate Shiv Jugular Carotid Arterial Slash

## Status
Accepted

## Context
In Butcher Bay and Dark Athena, stealth takedowns are vital for evading overwhelming guard firepower in maximum-security wings. Behind unaware sentries, Riddick can slip his blade around an enemy's throat and draw the edge horizontally across the carotid artery and jugular vein, dropping adversaries silently before an alarm can be sounded.

In flat desktop gaming, stealth kills were locked to contextual backstab prompts (`LMB` when undetected). In VR, tracking natural horizontal blade draw strokes across the cervical corridor empowers the player with visceral physical agency while strictly obeying anatomically accurate kill mechanics.

## Decision
1. **Target Cervical Corridor Proximity**:
   - Dominant controller positioned at neck/throat elevation ($Y_{rel} \in [-0.12, 0.16]\text{ m}, Z_{rel} \in [-0.34, -0.10]\text{ m}$).
2. **Horizontal Draw Stroke Dynamics**:
   - High lateral slashing velocity across the neck plane ($|V_x| > 1.25\text{ m/s}$) with grip sensor held ($> 0.50$).
   - Physical crouching stance (`s_isPhysicallyCrouched || s_isDeepProne`) to reflect predatory stealth approach.
3. **Arterial Release & Tissue Perforation Haptics**:
   - Synthesizes an initial sharp $260\text{ Hz}$ ($0.95$) skin incision click, transitioning into a rhythmic $110\text{ Hz}$ ($0.75$) arterial de-pressurization rush in the dominant hand.
4. **Engine Input Injection**:
   - Injects stealth execution sequence (`W` + `LMB`) with an $1800\text{ ms}$ lockout buffer.

## Consequences
- Turns stealth stalking into an exhilarating physical execution without cumbersome interface menus.
- High velocity requirement ($|V_x| > 1.25\text{ m/s}$) and specific throat corridor boundaries prevent false triggers during ordinary controller movement.
