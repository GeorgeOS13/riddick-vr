# ADR 0009: Physical Hand-over-Hand Ladder Climbing and Two-Handed Mantling Gestures

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*, vertical traversal is central to navigating high-security detention blocks, ventilation networks, industrial maintenance shafts, cargo hangars, and exterior ship scaffolding. Players frequently mount vertical ladders, descend into service pits, and mantle over ledge barriers.

### Reverse Engineering & Engine Architecture Findings
Analysis of `DarkAthena.exe`, `Content\GameClasses_Win32_x86.dll`, `MSystem.dll`, and `Content\Registry\RpgPlayer.xrg` revealed the traversal mechanics of the Starbreeze Athena Engine:
1. **Ladder Mounting & Attachment (`CLadderMapping`, `CRPG_Object_Player`):**
   - Ladders in the game world consist of vertical trigger volumes and spline normals.
   - Interacting with a ladder volume is initiated via `ACTION_USE 18` (DirectInput key `'E'`) or by pressing forward directly into the ladder's proximity envelope.
   - Upon mounting, Riddick transitions to the ladder traversal state (`ANIM_LADDER_CLIMB`), attaching the character collision capsule to the ladder plane and aligning movement along the vertical axis.
2. **Ascent and Descent Vector Translation:**
   - Movement along the ladder axis is controlled by forward/backward movement inputs:
     - `ACTION_FORWARD 17` (Key `'W'`): Propels the character upward along the ladder spline.
     - `ACTION_BACKWARD 31` (Key `'S'`): Propels the character downward along the ladder spline.
     - Releasing movement keys halts vertical movement, leaving the character holding the ladder in place.
3. **Ledge Mantling and Top Dismount (`ACTION_JUMP 57` / `VK_SPACE`):**
   - When reaching the upper threshold of a ladder or approaching a mantleable ledge, tapping Jump (`VK_SPACE`) or stepping forward executes the ledge vault/mantle animation (`ANIM_MANTLE_PULL`), hoisting Riddick's body onto the floor above.

### VR Immersion & Spatial Control Challenges
Relying solely on thumbstick inputs for ladder climbing in virtual reality causes cognitive dissonance and motion sickness, as the player's hands remain idle while their virtual body ascends. Players in modern VR titles expect:
1. **Spatial Rung Grabbing:** Reaching up to grip rungs above eye level with tactile haptic feedback.
2. **Pull-to-Climb Dynamics:** Physically pulling down on grabbed rungs to hoist the body upward, alternating hands naturally in a hand-over-hand rhythm.
3. **Controlled Descent:** Gripping and pushing upward to climb down safely.
4. **Two-Handed Mantling:** Squeezing and pulling down with both hands simultaneously at the top of a ladder to haul the character over the ledge onto the platform.
5. **Robust Input Disambiguation:** Grabbing ladder rungs with the right hand must never accidentally trigger weapon reload (`'R'`), boxer guard, or melee punches.

---

## Decision

We implemented a **Spatial Hand-over-Hand Ladder Climbing and Mantling System** in `src/hook/camera_hook.cpp`:

### 1. Spatial Rung Grabbing (`WorldToHmdLocal`)
OpenXR controller grip poses are continuously transformed into HMD-local space:
- $+X$: Player's right
- $+Y$: Up relative to gaze/eyes ($Y = 0$ at eye level)
- $-Z$: Forward in direction of sight

**Grab Criteria:**
- Hand raised above chin/shoulder level: $Y > 0.15\text{m}$ relative to HMD.
- Controller grip squeezed: `vrState.grip > 0.50f`.

**Initial Latch & Engine Mounting:**
When either hand transitions from free to grabbing:
1. A sharp tactile haptic pulse ($0.35$ amplitude) confirms the rung hold.
2. `SendKeyTap('E')` is dispatched, mounting and latching Riddick onto the ladder volume in the game engine.
3. The rung hold state (`s_leftHoldingRung` or `s_rightHoldingRung`) is latched.

**Sustained Hold Hysteresis:**
To allow a full, uninterrupted pulling stroke, the rung hold remains active as long as the grip button remains squeezed (`grip >= 0.40f`). This prevents the grab from disengaging prematurely when the hand is pulled down past eye level into chest or waist territory.

### 2. Pull-to-Climb Physics Translation
Linear controller velocities ($V_y$) in tracking space are sampled each frame:
- **Climbing Up (Pulling Down):** When holding a rung and pulling downward ($V_y < -0.25\text{ m/s}$), forward climb motion is dispatched (`s_ladderClimbUp = true` -> `SendKey('W', true)`). A continuous gentle haptic pulse ($0.20$ amplitude) provides tactile resistance.
- **Stroke Completion & Hand-over-Hand Rhythm:** When the downward pull slows below threshold ($V_y \ge -0.25\text{ m/s}$) or the grip is released, `'W'` is released. The player then reaches up with the opposite hand, grabs the next rung, and pulls down. Alternating between left and right hands produces a natural, rhythmic hand-over-hand ascent.
- **Climbing Down (Pushing Up):** Gripping and pushing upward ($V_y > +0.25\text{ m/s}$) dispatches backward climb motion (`s_ladderClimbDown = true` -> `SendKey('S', true)`), descending the ladder.

```
                  [Player Facing Ladder]
                            │
               Reaches Hand Up (Y > 0.15m)
                 & Squeezes Grip (> 0.5)
                            │
                            ▼
              ┌───────────────────────────┐
              │  LADDER RUNG GRABBED      │
              │  - 0.35 Haptic Pulse      │
              │  - SendKeyTap('E')        │
              └─────────────┬─────────────┘
                            │
            ┌───────────────┴───────────────┐
            │                               │
            ▼                               ▼
    [Single Hand Pull]              [Two-Handed Pull]
     Vy < -0.25 m/s                  Both Hands Vy < -0.25 m/s
            │                               │
            ▼                               ▼
  ┌───────────────────┐           ┌───────────────────┐
  │ PULL TO CLIMB     │           │ TWO-HANDED MANTLE │
  │ - SendKey('W')    │           │ - SendKeyTap(SPC) │
  │ - 0.20 Haptics    │           │ - 0.60 Haptics    │
  └───────────────────┘           └───────────────────┘
```

### 3. Two-Handed Mantling / Ledge Vault
When reaching the top lip of a ladder or ledge barrier:
- Both controllers hold rungs simultaneously (`s_leftHoldingRung && s_rightHoldingRung`).
- Both hands pull downward together ($V_{y,\text{left}} < -0.25\text{ m/s}$ and $V_{y,\text{right}} < -0.25\text{ m/s}$).
- A $1000\text{ms}$ debounce timer confirms eligibility.
- Dispatches `SendKeyTap(VK_SPACE)` and pulses both controllers with strong haptic feedback ($0.60$ amplitude).
- This triggers Riddick's ledge mantle vault, cleanly pulling the character over the top lip onto solid ground.

### 4. Input State Synthesis & Locomotion Coordination
Physical ladder climbing seamlessly integrates with thumbstick locomotion without key fighting or stuck input states:
```cpp
bool wantW = (vrState.leftController.thumbstickY > DEADZONE) || s_ladderClimbUp;
bool wantS = (vrState.leftController.thumbstickY < -DEADZONE) || s_ladderClimbDown;

if (wantW != s_wPressed) { SendKey('W', wantW); s_wPressed = wantW; }
if (wantS != s_sPressed) { SendKey('S', wantS); s_sPressed = wantS; }
```

### 5. Action Disambiguation & Isolation
While climbing or holding rungs (`isClimbingActive`):
- **Right Grip Reload (`'R'`):** Suppressed so gripping rungs with the right controller never triggers firearm reload.
- **Left Grip Interact (`'E'`):** Handled exclusively by rung grab events rather than holding down `'E'`.
- **Boxer Guard (`MOUSEEVENTF_RIGHTDOWN`):** Suppressed so raised fists holding rungs do not enter defensive blocking stance.
- **Melee Punches:** Suppressed so fast climbing pulls are not misinterpreted as jabs or uppercuts.
- **Tactical Reloads & Two-Handed Stabilization:** Suppressed while hands are engaged on the ladder.
- **Eyeshine Temple Tap:** Suppressed during climbing to avoid accidental vision toggles.

---

## Consequences

### Positive
- **High Physical Immersion:** Intuitive, visceral vertical traversal using natural body mechanics.
- **Zero Input Conflicts:** Weapon reload, boxer guard, and melee punching remain completely isolated from climbing.
- **Tactile Feedback:** Multi-tier haptic pulses communicate rung grabs ($0.35$), active pulling strokes ($0.20$), and mantle vaults ($0.60$).
- **Seamless Fallback:** Thumbstick locomotion remains fully functional and smoothly blends with gestural climbing.

### Verification
- **Build Verification:** Successfully built with `powershell -ExecutionPolicy Bypass -File tools/build-all.ps1`.
- **Autonomous Playback Pipeline:** Validated with `python tools/self_test_pipeline.py --frames 60 --interval 15 --timeout 25`, achieving zero glitches and full frame stability.
