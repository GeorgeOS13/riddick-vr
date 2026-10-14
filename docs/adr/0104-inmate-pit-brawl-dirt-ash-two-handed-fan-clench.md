# ADR 0104: Inmate Pit Brawl Dirt / Ash Two-Handed Fan Clench & Burst

## Status
Accepted

## Context
In Butcher Bay pit brawls, an experienced brawler can gather double handfuls of abrasive concrete dust, crushed coal slag, and floor soot into both cupped palms (ADR 0096). By bringing both clenched fists together at the sternum and explosively flinging them outward horizontally, the fighter creates an impenetrable particulate cloud that blinds an entire front arc of surrounding inmates.

In conventional monitor gameplay, area-of-effect crowd control required specialized grenade weapons. In VR, leveraging dual-controller tracking and opposing bilateral arm velocities delivers visceral bare-knuckle arena crowd control.

## Decision
1. **Pre-Clench Chest Compression**:
   - Both hands brought together at chest elevation ($dist < 18\text{ cm}, Y_{rel} \in [-0.25, 0.05]\text{ m}$) with both grips clenched firmly ($Grip_{left} > 0.60 \land Grip_{right} > 0.60$).
2. **Explosive Bilateral Fan Fling**:
   - Arms violently thrust outward laterally in opposing directions:
     - Left controller velocity: $V_{x, left} < -1.10\text{ m/s}$.
     - Right controller velocity: $V_{x, right} > 1.10\text{ m/s}$.
3. **Dual Particulate Cloud Rush Haptics**:
   - Initial $200\text{ Hz}$ ($0.45$) friction grind during the hand clench, followed by a wide-spectrum $175\text{ Hz}$ ($0.70$) particulate wind rush across both hands ($90\text{ ms}$ duration).
4. **Engine Input Injection**:
   - Dispatches wide-area blind sequence (`E` + `LMB` + `VK_TAB`) with a $2000\text{ ms}$ re-gather deadband.

## Consequences
- Empowers the player to manage multi-target swarms during intense bare-knuckle pit arena fights.
- Strict opposing velocity constraints ($|V_x| > 1.10\text{ m/s}$) ensure no cross-interference with Boxer Guard blocks or one-handed hooks.
