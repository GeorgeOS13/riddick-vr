# ADR 0017: Interactive Physical Object Throwing and Kinematic Release Propulsion

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena*, Riddick frequently manipulates environmental physics objects:
1. **Explosive Gas Canisters:** Transporting volatile fuel cylinders to breach barricades or detonate near clusters of mercenaries.
2. **Debris, Crates & Severed Heads:** Grabbing dismembered droid components or severed guard heads to pass biometric gate scanners or trigger laser tripwires from a distance.
3. **Subdued Enemy Bodies:** Dragging unconscious or deceased inmates and guards into shadows to avoid detection by patrol drones.

In the original PC title:
- Holding the interact key (`E`) picks up and hoists the targeted object.
- Re-pressing or releasing `E` gently deposits the object at Riddick's feet.
- Pressing `Left Mouse Button` (Primary Attack) while carrying an object activates the engine's physical propulsion impulse, launching the object forward along Riddick's crosshair.

In Virtual Reality, requiring a player to pull a weapon trigger or click a mouse to throw an object they are physically holding with their motion controller is immersion-shattering. Players instinctively wind up, swing their arm forward, and release the controller grip button at the apex of the swing to release and propel the object.

## Decision

We implement a **Kinematic Release Evaluator** within Section 9 of [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Off-Hand Controller Grip State Transition]
                                       │
                      ┌────────────────┴────────────────┐
                      ▼                                 ▼
             [Grip Pressed: Grab]             [Grip Released: Drop / Throw]
             - Dispatch 'E' (Down)                      │
             - Latch Haptic Pulse (0.35)                ▼
                                            [Evaluate Controller Speed]
                                                        │
                              ┌─────────────────────────┴─────────────────────────┐
                              ▼                                                   ▼
                    [Speed <= 1.2 m/s]                                   [Speed > 1.2 m/s]
                     (Stationary / Gentle)                                (Dynamic Throw)
                              │                                                   │
                              ▼                                                   ▼
                     - Dispatch 'E' (Up)                                 - Dispatch Mouse Left Click
                     - Gentle drop at feet                               - Native Starbreeze throw impulse
                                                                         - Release Haptic Impulse (0.60)
```

### 1. Velocity-Thresholded Throwing Dispatch
- When the player releases the Left Grip button (`!wantLeftGrip && s_leftGripDown`), the system samples the linear velocity vector $\vec{V}_{left}$ and magnitude $S_{left} = \|\vec{V}_{left}\|_2$.
- **Dynamic Throw ($S_{left} > 1.2\text{ m/s}$):**
  If the arm is swung with meaningful forward/upward throwing velocity, the hook dispatches a synthetic `MOUSEEVENTF_LEFTDOWN` / `MOUSEEVENTF_LEFTUP` event. The engine treats this as an attack release, imparting maximum forward kinetic momentum along the player's view vector.
- **Gentle Drop ($S_{left} \le 1.2\text{ m/s}$):**
  If the controller is stationary or moved gently below $1.2\text{ m/s}$, the hook dispatches `SendKey('E', false)`, cleanly setting the object down without making noise or alerting nearby guards.

### 2. Multi-Band Haptic Feedback
- **Grab Latch:** A short $0.35$ amplitude tactile pulse ($85\text{ Hz}$) signals when Riddick grips an object.
- **Kinematic Release:** A sharp $0.60$ amplitude impulse ($120\text{ Hz}$) confirms the successful launch of an airborne projectile.

### 3. Safety Invariants & State Isolation
- Throwing evaluation is fully bypassed during ladder climbing (`isClimbingActive`) and two-handed weapon stabilization (`s_twoHandedStabilizationActive`) to avoid unintended interaction conflicts.

## Consequences

### Positive
- Visceral, natural physical throwing of explosive fuel canisters, rocks, and debris.
- Intuitive differentiation between stealthy drops and aggressive forward throws based entirely on physical arm kinematics.
- Zero extra buttons or UI menus required for physics interactions.
