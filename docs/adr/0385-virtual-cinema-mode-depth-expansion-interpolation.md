# ADR 0385: Virtual Cinema Mode Depth & Scale Expansion Transition Interpolation

## Status
Accepted

## Context
In *The Chronicles of Riddick* VR mod, Virtual Cinema mode (`XrCompositionLayerQuad`) is used for floating menus, cutscenes, and loading screens at $1.8\text{ m}$, while action gameplay uses full stereoscopic 3D projection (`XrCompositionLayerProjection`).

Previously, switching between cinema mode and action mode was an instantaneous, single-frame layer cut:
```cpp
if (clientState.cinemaMode) {
    layers.push_back(&quadLayer);
} else {
    layers.push_back(&layer);
}
```
This abrupt swap caused a visible single-frame pop and depth jump when skipping cutscenes or exiting in-game menus into combat.

## Decision
Implement smooth depth and scale expansion interpolation in `src/host/openxr_manager.cpp`:

1. **Normalized Transition Alpha**:
   - Maintain a normalized transition variable: $s_{\text{cinemaTransitionAlpha}} \in [0.0, 1.0]$.
   - Step alpha by $\pm 0.08$ per frame ($\approx 150\text{ ms}$ transition at 72–90 Hz).
   - Evaluate smoothstep curve: $t = \alpha^2 (3 - 2\alpha)$.

2. **Depth & Scale Expansion**:
   - While transitioning out of cinema mode ($t \to 0$):
     - Screen distance glides from $1.80\text{ m}$ forward to $0.85\text{ m}$.
     - Screen width smoothly expands from $2.0\text{ m}$ to $3.6\text{ m}$ ($16:9$ aspect preserved).
     - Both the base projection layer and the expanding quad overlay are rendered simultaneously during the intermediate frames.
   - When $t \le 0.001$, the quad layer is cleanly retired, leaving the fully immersive 3D scene.
   - While transitioning into cinema mode ($t \to 1$), the screen gracefully glides back to $1.80\text{ m}$.

## Consequences
- Completely eliminates single-frame visual pops and depth jarring during menu/cutscene transitions.
- Creates an elegant "screen expansion" cinematic portal effect when entering action gameplay.
- Preserves 100% full viewport immersion in action mode with zero black bars.
