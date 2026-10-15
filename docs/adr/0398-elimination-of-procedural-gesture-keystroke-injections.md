# ADR 0398: Elimination of Procedural Gesture Keystroke Injections & Stick Navigation Isolation

## Context
During aggressive physical combat and rapid controller arm movements, players experienced erratic game state transitions, accidental jumps, unintended crouches, and sudden dialogue/PDA popups.
Analysis of `CameraHook::Update` revealed two major sources of spurious key events:
1. Procedural Environmental Gestures: A legacy 3000-line block of sequential position/velocity checks (originally labeled ADR 0028–0370) continuously monitored hand coordinates for complex compound thresholds and dispatched rapid bursts of keyboard inputs (`SendKeyTap('E')`, `SendKeyTap('Q')`, `SendKeyTap(VK_TAB)`, `SendKeyTap(VK_RETURN)`, `SendKeyTap(VK_SPACE)`). When throwing punches or swinging weapons, random arm trajectories frequently matched these loose bounding boxes, flooding the engine input queue.
2. Ungated Right Stick Y Navigation: Right thumbstick vertical deflection ($|Y| > 0.55$) dispatched `VK_UP` and `VK_DOWN` arrow keys on every tilt without checking whether the player was in a menu or active 3D gameplay.
3. Hip-Level Trigger Burst: Resting hands downward near the waist while pulling triggers executed an accidental jump (`VK_SPACE`) and forward stride ('W').

## Decision
1. Eliminate all legacy procedural machine gesture checks and their associated key bursts. Only authentic game interactions are retained: physical vent grate prying (dual-hand pull), biometric terminal scanner contact (index poke), and NanoMED regeneration station healing (flat open palm).
2. Isolate right thumbstick vertical arrow key navigation strictly to Cinema Mode menus (`if (vrState.cinemaMode)`), preventing spurious arrow inputs during 3D action gameplay.
3. Remove the hip-level trigger jump injection.

## Consequences
- Reduces compiled hook DLL binary size from 270 KB to 193 KB (eliminating 76 KB of unused instruction bloat).
- Completely prevents phantom keypress bursts, accidental jumps, menu disruptions, and UI popups during intense VR combat.
- Game input remains 100% predictable, responsive, and tactile.
