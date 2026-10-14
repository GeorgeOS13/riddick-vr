# ADR 0011: Granular Melee Combat, Dual-Blade Ulak Cross-Slashes, and Stealth Execution Gestures

## Status
Accepted

## Context
Melee combat in *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena* is renowned for its cinematic grit, weight, and lethality. Riddick's arsenal spans bare fists, brass knuckles, improvised prison shivs, screwdrivers, clubs, and the signature dual Ulaks (curved serrated forearm blades).

Prior ADRs established:
- Basic velocity-threshold punches (jabs, hooks, uppercuts) and boxer guard stance (ADR 0005, 0008).
- Physical ladder climbing and mantle hoists (ADR 0009).
- Adaptive stealth wall-peeking and diegetic weapon HUDs (ADR 0010).

However, flat-screen melee inputs relied on directional key combos (`W/A/S/D` + Attack) to trigger distinct strikes (e.g. forward thrusts, side slashes, overhead heavies, and stealth neck snaps). In VR, players expect their natural arm kinematics—such as crossing both blades in front of their chest, driving a shiv downward into an opponent, or slicing a blade across a guard's throat from behind—to trigger matching lethal combat moves.

## Decision

We implement an extended **Kinematic Melee Classification & Gestural Execution Engine** in `src/hook/camera_hook.cpp`:

```
                    [Dual-Controller Spatial Kinematics]
                                      │
        ┌─────────────────────────────┼─────────────────────────────┐
        ▼                             ▼                             ▼
[Dual Ulak Cross-Slash]     [Overhead Shiv Plunge]       [Silent Stealth Takedown]
- Convergent inward Vx      - High Vy (< -1.35 m/s)      - Crouched / Peeking stance
  (Rx < -1.1 & Lx > 1.1)    - Starts above shoulder      - Hand extended (Z < -0.25m)
- Dispatches 'A' + 'D'      - Dispatches 'S' + Attack    - Dispatches 'E' + Attack
- Dual 0.95 Haptics         - 0.95 Haptic plunge         - 0.90 Tactile throat-slit
        │                             │                             │
        └─────────────────────────────┼─────────────────────────────┘
                                      ▼
                        [Starbreeze Combat Engine]
                        - 260ms Fluid Combo Window
                        - 0ms Guard Interruption
                        - Haptic Impulse Modulation
```

### 1. Dual Ulak Scissor Cross-Slash (X-Slash)
- **Kinematic Detection:**
  Evaluates horizontal velocity convergence across both controllers:
  $$\text{dualCrossSlash} = (V_{x, right} < -1.1\text{ m/s} \land V_{x, left} > +1.1\text{ m/s}) \lor (V_{x, right} > +1.1\text{ m/s} \land V_{x, left} < -1.1\text{ m/s})$$
- **Engine Translation:**
  Dispatches rapid alternating left and right strikes (`A` + Attack, followed by `D` + Attack), executing the Starbreeze dual-blade scissor decapitation combo.
- **Haptic Delivery:** Simultaneous maximum-intensity tactile pulses ($0.95$ amplitude) on both controllers.

### 2. Overhead Shiv Plunge & Grounded Finisher
- **Kinematic Detection:**
  Detects downward plunging drive ($V_y < -1.35\text{ m/s}$) initiating from shoulder/chest level ($Y_{local} > -0.10\text{ m}$) or while physically crouched.
- **Engine Translation:**
  Dispatches backward heavy attack (`S` + Attack), mapping to the downward stabbing animation and grounded impalement finisher.
- **Haptic Delivery:** Heavy impact pulse ($0.95$ amplitude) to the striking hand.

### 3. Silent Stealth Takedown / Throat-Slit Gesture
- **Kinematic Detection:**
  Active when the player is physically crouched or in stealth corner-peek stance (`s_isPhysicallyCrouched || s_isStealthPeeking`), with the striking hand extended forward ($Z_{local} < -0.25\text{ m}$) and delivering a forward thrust ($V_z < -1.1\text{ m/s}$) or lateral slice ($|V_x| > 1.1\text{ m/s}$).
- **Engine Translation:**
  Dispatches interaction + primary attack (`SendKeyTap('E')` + Attack), initiating Riddick's contextual stealth assassination (throat slit with blade, or neck twist when unarmed).
- **Haptic Delivery:** Crisp tactile vibration ($0.90$ amplitude).

### 4. Directional Slashes & Fluid Combo Synchronization
- **Lateral Slashes:** High-velocity lateral swings ($|V_x| > 1.2\text{ m/s}$) trigger left/right horizontal slashes (`A`/`D` + Attack) with $0.90$ haptic feedback.
- **Forward Thrusts & Straight Punches:** Linear forward drives trigger lunging blade thrusts / straight jabs (`W` + Attack).
- **Reduced Combo Recovery:** Decreased gesture lockout from $320\text{ ms}$ to $260\text{ ms}$, aligning with Starbreeze's 3-hit combo window to allow rapid fluid chained strikes.
- **Instant Guard Break:** Any strike instantly disengages the boxer guard ($0\text{ ms}$ delay) to ensure attacks are never blocked or delayed by player stance transitions.

## Consequences

### Positive
- Fully realized gestural melee combat authentic to Riddick's brutality with dual Ulaks and improvised shivs.
- Natural stealth takedown execution directly from room-scale crouching and wall-peeking cover.
- Highly responsive 3-hit combo chaining with distinct tactile signatures for blades, fists, and execution strikes.
