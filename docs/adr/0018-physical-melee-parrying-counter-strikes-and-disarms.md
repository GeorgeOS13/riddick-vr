# ADR 0018: Physical Melee Parrying, Kinematic Counter-Strikes, and Reactive Disarms

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena*, hand-to-hand combat features an advanced counter-attack and disarm system. When facing armed guards (shotguns, assault rifles, clubs) or vicious inmates (shivs, knuckles):
- If the player holds Block (`Right Mouse Button`) and inputs Primary Attack (`Left Mouse Button`) within the exact window of an enemy's incoming swing, Riddick intercepts the attack:
  - **Gun Disarm:** Grabs the barrel, twists the weapon from the guard's grip, forces the stock upward under their chin, and fires point-blank.
  - **Shiv / Knife Counter:** Deflects the blade, snaps the attacker's wrist or elbow, and thrusts the blade through their jugular.
  - **Unarmed Counter:** Ducks a punch, hooks the attacker's shoulder, and drives them into the ground.

In the original PC controls, this was executed via rapid button coordination (`RMB` held + `LMB` clicked). In Virtual Reality, requiring a player to pull a weapon trigger or click a mouse while raising their fists in a physical boxer guard breaks immersion. When an enemy lunges in VR, the player naturally thrusts their fists or forearm forward into the blow to deflect and counter.

## Decision

We implement a **Kinematic Counter Evaluator** in Section 5a of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Player Holding Boxer Guard Pose]
                   (s_currentBlockEngaged = true -> RMB Down)
                                   │
                                   ▼
             [Enemy Attacks -> Player Thrusts Fists Forward]
             - Hand Local Z < -0.15m (in front of chest)
             - Forward Velocity: Vz < -1.15 m/s
                                   │
                  ┌────────────────┴────────────────┐
                  ▼                                 ▼
       [Forward Parry Shove]               [Standard Melee Strike]
       - KEEP RMB (Block) Held             - Disengage Block (RMB Up)
       - Dispatch LMB Click                - Dispatch Directional Attack
       - Engine triggers Counter-Disarm!   - Normal Punch / Hook
       - High-Resonance Shockwave (0.95)   - Standard Punch Haptic (0.90)
```

### 1. Guard-Preserved Primary Fire Dispatch
- When `s_currentBlockEngaged` is active and either controller exhibits aggressive forward linear velocity ($V_z < -1.15\text{ m/s}$ while $Z_{local} < -0.15\text{ m}$), the hook dispatches a synthetic `MOUSEEVENTF_LEFTDOWN` / `MOUSEEVENTF_LEFTUP` sequence.
- Crucially, `MOUSEEVENTF_RIGHTDOWN` is NOT dropped during this thrust. This satisfies Starbreeze's engine requirement: `ACTION_PRIMARYFIRE` registered while `ACTION_SECONDARYFIRE` is held enters `STATE_COUNTER_DISARM`.
- If the enemy is within the strike window, the cinematic disarm animation triggers instantly. If no enemy is attacking, Riddick executes a quick defensive guard jab.

### 2. High-Frequency Tactile Shockwave
- Dispatches a peak amplitude $0.95$ tactile pulse to both controllers.
- OpenXR Host synthesizes a high-frequency $280\text{ Hz}$ shockwave ($60\text{ ms}$) simulating bone-cracking physical contact.

### 3. Separation from Standard Punches
- Wide lateral hooks ($|V_x| > 1.2\text{ m/s}$) or vertical uppercuts ($V_y > 1.2\text{ m/s}$) continue to release the guard immediately ($0\text{ ms}$) to ensure fluid combo striking. Only direct forward thrusts from guard execute the counter sequence.

## Consequences

### Positive
- Players can instinctively deflect incoming guard attacks and wrench firearms away purely through physical reaction.
- Zero extra buttons or unnatural key combinations.
- Complete fidelity to the original brutal Starbreeze melee combat design.
