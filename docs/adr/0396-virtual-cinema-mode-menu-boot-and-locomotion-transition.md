# ADR 0396: Virtual Cinema Mode Menu Boot and Locomotion Auto-Transition

## Context
Players reported that black bars were present in-game (bad) but missing in the main menus (bad). The root causes were:
1. In `opengl_proxy.cpp`, the 3D perspective projection threshold check used `m[0] > 1.20f` to classify telephoto cutscenes. Because Dark Athena's standard 74° gameplay camera has $m[0] = 1.33f$, normal gameplay was incorrectly classified as telephoto cinema mode.
2. When cinema mode was active in gameplay, `stereo_renderer.cpp` cleared borders to solid black and letterboxed to 16:9, causing black bars inside the VR headset.
3. Dark Athena hides the OS cursor in full screen, so cursor-based menu detection failed to detect main menus on startup, causing menus to render in distorted full-screen projection instead of the floating cinema quad.

## Decision
1. In `opengl_proxy.cpp`, correctly classify standard 3D perspective projections ($m[0] \in [0.10, 4.00]$, $|m[15]| < 0.01$), ensuring standard 70–75° gameplay projections ($m[0] \approx 1.33$) are recognized as 3D gameplay.
2. Initialize `s_cinemaModeState = true` on game startup so the initial boot and main menu render comfortably on the floating 16:9 Virtual Cinema Quad at 1.8m.
3. Automatically transition from Cinema Mode to Full 3D VR Immersion (`s_cinemaModeState = false`) the instant the player deflects the locomotion thumbstick ($|L_x| > 0.35$ or $|L_y| > 0.35$) or right look stick ($|R_x| > 0.35$).
4. Automatically re-engage Cinema Mode when the player presses the Left Controller Menu Button to open the pause menu.
5. Retain Button Y as an instantaneous manual toggle between Virtual Cinema Quad and Full 3D VR Immersion at any time.

## Consequences
- The main menu and pause menus render cleanly on a floating 16:9 cinema quad screen in front of the player.
- In-game action mode blits edge-to-edge ($0, 0, W, H$) with native headset lens FOV, completely eliminating in-game black bars (100% full VR immersion).
- Smooth, natural transition between menus and gameplay without relying on fragile projection matrix heuristics.
