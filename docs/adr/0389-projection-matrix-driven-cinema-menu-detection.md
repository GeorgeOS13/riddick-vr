# ADR 0389: Projection Matrix Driven Cinema Menu & Cutscene Detection

## Context
Automatic switching between immersive 3D action projection and the floating 16:9 virtual cinema quad previously relied on Windows OS cursor visibility (`CURSOR_SHOWING`).
However, *The Chronicles of Riddick: Assault on Dark Athena* suppresses the OS cursor in full-screen mode via `ShowCursor(FALSE)` and renders its own in-engine software cursor. Consequently, `cursorShowing` remained false in the main menus, preventing `cinemaMode` from engaging and leaving the main menus stretched or improperly displayed without the floating cinema screen.

## Decision
1. Enhance `OpenGLProxy` to track 3D gameplay perspective projections:
   - Identify orthographic projections (`proj[15] == 1.0f`) used by UI screens, menus, and loading panels.
   - Identify telephoto narrow-FOV projections (`proj[0] > 1.20f`) used in cinematic cutscenes and scripted dialogues.
   - Flag 3D first-person gameplay projections (`proj[0] <= 1.20f && proj[15] == 0.0f`).
2. Automatically engage `cinemaMode` if:
   - No 3D gameplay projection has been loaded yet (initial main menus, loading screens), OR
   - The active projection matrix is orthographic or telephoto cutscene, OR
   - The OS cursor is visible, OR
   - The player manually toggled cinema mode via Button Y.

## Consequences
- Main menus reliably render as a floating 16:9 virtual cinema screen at 1.8m with black surrounding void.
- First-person gameplay automatically transitions seamlessly into 100% full-immersion 6DOF VR without manual intervention.
