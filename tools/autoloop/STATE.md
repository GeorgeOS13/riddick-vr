# RiddickVR Autoloop — State

Durable, human-readable state written by each scheduled wake. Newest at top.

> **COORDINATION (updated):** the `.xmd` parser (`tools/mos_xmd.py`) and ADR-0413
> are committed. Ghidra 12.1.4 is installed at
> `C:\Users\GigelCastel\Tools\ghidra_12.1.4_PUBLIC` and `GHIDRA_INSTALL_DIR` is
> set, so `rea` can now decompile. Issues #55 (decode MOS geometry/skinning →
> real hand mesh), #56 (pin target build), #57 (VR portability epic) are open.
> Loop should now prefer #55 and the #57 gaps; see below.

## Wake 1 — 2026-10-15 (interactive, setup + RE)
- Provisioned **Ghidra 12.1.4** (`C:\Users\GigelCastel\Tools\ghidra_12.1.4_PUBLIC`,
  `GHIDRA_INSTALL_DIR` set). `rea doctor` now reports ghidra healthy → real
  decompilation is available to `rea` (CLI) and future wakes.
- Committed `tools/mos_xmd.py` — read-only `MOS DATAFILE2.0` `.xmd` parser,
  validated on Riddick/weapon models (node tree + chunk offsets/sizes +
  triangle-count cross-check).
- Wrote `docs/adr/0413-native-first-person-hand-model-reuse.md`.
- Opened issues: **#55** (decode MOS geometry/skinning → real hand mesh),
  **#56** (pin target build: EFBB 2004 vs AoDA 2009), **#57** (VR portability
  epic). Commented the plan on **#46**.
- Next (loop): start **#55** — use `rea`/Ghidra to xref the `MOS DATAFILE2.0`
  loader in `MXR.dll`/`MSystem.dll`, recover `VERTEXFRAMES`/`BONEDEFORM`/`NODES`
  layouts, then extract the hand/forearm mesh.

## Wake 0 — 2026-10-15 (setup)
- Created the autoloop harness (`tools/autoloop/`): standing prompt, runner
  script, scheduled task `RiddickVR-Autoloop` every 10 min for 24h.
- Investigation so far (from the interactive session):
  - Repo `main` clean at `25f90b3`. Open issues: #34, #40, #43-#54.
  - Renderer is a **monoscopic capture+blit** (`stereo_renderer.cpp`) that draws
    the procedural `VRHandRenderer` overlay into each eye FBO. Hands are NOT
    native (ADR-0406/0407 acknowledge this).
  - `src/hook` intercepts only `wgl*`, `glViewport`, `glMatrixMode`,
    `glLoadMatrixf`, `glFrustum`, `glOrtho`, `glClear`, `glBindFramebuffer`,
    and `GDI32!SwapBuffers` (IAT). `glUniformMatrix4fv` is NOT hooked despite
    README claim. Matrix concatenation paths (`glPush/Pop/Translate/Rotate`,
    `glMultMatrixf`) are pure forwarders => invisible.
  - `CameraHook::StabilizeViewMatrix` (camera_hook.cpp:228) and
    `OpenGLProxy::GetLastViewMatrix` are dead/unwired.
  - Game install found: `C:\Program Files (x86)\Starbreeze Studios\Riddick EFBB`
    = **2004 original** (`Riddick.exe`, `SbzEngine.exe`), NOT the 2009 AoDA
    (`DarkAthena.exe`) the `config/addresses.json` recon targeted. GOG AoDA
    installer present but unextracted in Downloads.
  - `.xmd` = `MOS DATAFILE2.0` container with `SKELETON`/`NODES`/`BONEDEFORM`/
    `TRIANGLES`/`PRIMITIVES`/`VERTEXFRAMES` chunks and texture refs
    (e.g. `RIDDICK_ARMSCAR_D`). Real geometry + skinning appear extractable.
  - REA 6.5.0 CLI present; no Ghidra/IDA/Hopper configured (static PE/format
    analysis only for now).
- Next steps: build a read-only `.xmd` (MOS DATAFILE2.0) parser to extract the
  Riddick arm/hand mesh + skeleton; identify the correct first-person arm model;
  produce an ADR + tool + test; then design the native-hand render path behind a
  config flag with the procedural renderer retained as fallback.
