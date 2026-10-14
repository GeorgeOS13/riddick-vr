# ADR 0054: Inmate Pit Brawl Stool / Chair Blunt Weapon Wrench & Smash

## Status
Accepted / Implemented

## Context
During the brutal pit fights and recreation hall skirmishes in Butcher Bay (Courtyard, Aquila's Territory, Abbott's quarters), inmates frequently weaponize heavy steel stools, cafeteria benches, and metal furniture. Wrenching bolted metal furniture from concrete decks provides high-damage improvised blunt weaponry capable of shattering guard armor and knocking down opponents.

In VR, grabbing and ripping an anchored steel stool from floor bolts must feel weighty, requiring physical force, followed by devastating two-handed crushing swings.

## Decision
We implement physical stool wrenching and two-handed blunt weapon smashing in Section 6 and Section 8/9 of `src/hook/camera_hook.cpp`:
1. **Low Furniture Reach & Dual Grip**:
   - Player reaches down toward floor furniture level:
     $Y_{local} \in [-0.70, -0.30]\text{ m}$, $Z_{local} \in [-0.45, -0.15]\text{ m}$.
   - Both controller grips clenched ($> 0.60$).
2. **Kinematic Upward Wrenching Gesture**:
   - Upward arm jerk exceeding threshold:
     $V_{y, left} > 1.05\text{ m/s} \land V_{y, right} > 1.05\text{ m/s}$.
   - Dispatches interact & weapon equip Slot '1' (`E` + `'1'`), freeing the stool from floor rivets.
   - Triggers explosive $200\text{ Hz}$ ($0.85$ amplitude) shearing crack haptic across both hands.
3. **Heavy Improvised Blunt Strike**:
   - In two-handed stance, swings exceeding high kinetic velocity ($|V_{\text{combined}}| > 1.35\text{ m/s}$):
   - Dispatches Primary Attack (`LMB` down/up), delivering Starbreeze's crushing blunt impact with dual $0.95$ ($120\text{ Hz}$) bone-shattering recoil.

## Consequences
- Deepens prison brawl sandbox mechanics with tactile environmental improvised weaponry.
- Distinct upward wrenching motion prevents unintended activation during standard crouching or locomotion.
- High velocity swing requirement preserves the weighty inertia of heavy metal furniture.
