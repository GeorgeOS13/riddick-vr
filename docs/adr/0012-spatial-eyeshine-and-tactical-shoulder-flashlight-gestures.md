# ADR 0012: Spatial Eyeshine, Tactical Shoulder Flashlight Gestures, and Gauntlet Status Integration

## Status
Accepted

## Context
*The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena* feature two critical optical mechanisms for navigating pitch-black environments:

1. **Eyeshine Night Vision (`Q`):**
   Riddick's surgical night-vision adaptation providing silver-blue luminance amplification and enhanced silhouette detection in shadows without emitting detectable light to enemies.
2. **Tactical Shoulder Flashlight (`F`):**
   A focused incandescent beam mounted on the left tactical harness, providing strong illumination in exchange for exposing player position.

In flat-screen gameplay, these tools were bound to separate keyboard shortcuts (`Q` and `F`). Previous VR implementations unified these into a generic head-proximity tap, which created tactical ambiguity (e.g. accidentally turning on the conspicuous shoulder flashlight when attempting to activate silent eyeshine in stealth).

## Decision

We implement **Spatially Differentiated Optical Gestures** and an **Expanded Diegetic Biomonitor** in `src/hook/camera_hook.cpp` and `src/host/openxr_manager.cpp`:

```
                 [Player Motion Controller Poses]
                                │
        ┌───────────────────────┴───────────────────────┐
        ▼                                               ▼
[Right Hand -> Temple Tap]             [Left Hand -> Shoulder Tap]
- Pos: Y > +0.08m, X > +0.12m          - Pos: Y > +0.06m, X < -0.12m
- Grip squeezed (> 0.50f)              - Grip squeezed (> 0.50f)
- Toggles Eyeshine ('Q')               - Toggles Shoulder Flashlight ('F')
- High-frequency optical buzz (0.85)   - Mechanical switch click (0.75)
        │                                               │
        └───────────────────────┬───────────────────────┘
                                ▼
               [Left Gauntlet Biomonitor (14cm x 7cm)]
               - 4-Box Segmented Health Status
               - Eyeshine & Shadow Concealment Indicator
               - 0.26 x 0.16 Swapchain Sub-Image Crop
```

### 1. Spatial Optical Gesture Differentiation (`src/hook/camera_hook.cpp`)
- **Eyeshine Night Vision (Right Temple):**
  When the player raises their right hand to the right temple ($Y_{local} > 0.08\text{ m}, X_{local} > +0.12\text{ m}$) with grip squeezed ($> 0.50$), the system toggles native Eyeshine (`SendKeyTap('Q')`) and delivers a high-resonance optical vibration ($0.85$ amplitude).
- **Tactical Shoulder Flashlight (Left Shoulder):**
  When the player raises their left hand to the left shoulder harness ($Y_{local} > 0.06\text{ m}, X_{local} < -0.12\text{ m}$) with grip squeezed ($> 0.50$), the system toggles the tactical shoulder beam (`SendKeyTap('F')`) with a solid mechanical tactile impulse ($0.75$ amplitude).
- **Temporal Debounce & Climbing Isolation:**
  A $1200\text{ ms}$ re-trigger buffer prevents double-toggling. Both gestures are locked out during active ladder climbing (`isClimbingActive`).

### 2. Expanded Gauntlet Status Biomonitor (`src/host/openxr_manager.cpp`)
- The left gauntlet `wristQuadLayer` is resized to $14\text{ cm} \times 7\text{ cm}$, cropping a $26\% \times 16\%$ region from the upper-left swapchain buffer.
- This encompasses both the 4-segment health meter and the native eyeshine / stealth shadow concealment icon, providing complete biomonitor telemetry on the player's forearm gauntlet.

## Consequences

### Positive
- Clear, intuitive physical separation between silent stealth night vision and conspicuous flashlight illumination.
- Fully diegetic forearm biomonitor showing both health vitality and optical stealth status.
- Zero input collisions with ladder climbing, boxing guard, or weapon stabilization.
