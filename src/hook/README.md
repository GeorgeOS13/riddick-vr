# 32-bit Hook Component (`opengl32.dll`)

## Responsibility
The Hook component is a 32-bit (x86) dynamic link library compiled as `opengl32.dll` (or an injected payload DLL). It loads directly into `DarkAthena.exe` within the game's address space.

## Key Duties
- Export standard WGL entry points (`wglCreateContext`, `wglDeleteContext`, `wglMakeCurrent`, `wglSwapBuffers`, `wglGetProcAddress`).
- Forward all unhooked OpenGL calls to the real system `C:\Windows\SysWOW64\opengl32.dll`.
- Intercept viewport setup (`glViewport`, `glScissor`) and matrix uploads (`glUniformMatrix4fv`).
- Manage the double-pass stereo render loop:
  1. Left eye render target setup & matrix injection.
  2. Right eye render target setup & matrix injection.
- Pass rendered texture handles and receive tracking data from the Bridge component.

## Native first-person hand mesh (GH #46)
- `native_hand_mesh.h` + generated `native_hand_mesh_data.h`: the real Riddick
  arm/hand geometry (baked from the shipped `.xmd` models by `tools/xmd_bake.py`)
  as a compile-in, dependency-free asset. Header-only; unit-tested by
  `native_hand_mesh_asset` (`tests/native_hand_mesh_tests.cpp`).
- `VRHandRenderer` still draws the **procedural** overlay by default; the baked
  asset is consumed by the native draw path behind the INI flag
  `[VR] NativeHands` (see ADR-0420/0427). Do not claim the procedural hands are
  the game's real model. The native path is a provisional, rigid, headset-tuning
  draw — the procedural overlay remains the fallback.
- `native_hand_pose.h` maps the asset's model-space AABB into the hand-local
  frame (unit-tested by `native_hand_pose_mapping`,
  `tests/native_hand_pose_tests.cpp`).

## Native hand diffuse texture (GH #61)
- `native_hand_texture.h` + generated `native_hand_texture_data.h`: the real
  Riddick skin/scar diffuse (`Riddick_ArmScar_D`) baked from the shipped
  `Content/Models.xtc` by `tools/xtc_bake.py`, stored as the engine's own
  **DXT1/BC1** blocks (no re-encode).
- `bc1.h` decodes those blocks to RGBA8 (`VRHandRenderer::DrawNativeHand`
  uploads a fixed-function `GL_TEXTURE_2D` and modulates it under the existing
  lit colour); unit-tested by `native_hand_texture_asset`
  (`tests/native_hand_texture_tests.cpp`), which cross-checks the C++ decoder
  against reference pixels + checksum produced by the independent Python decoder
  in `tools/mos_xtc.py`.
- Gated by `[VR] NativeHands` (mesh) and `[VR] NativeHandTexture`; the flat
  skin tone is retained as the fallback. Only active when `NativeHands` is set.


