# RiddickVR Autoloop — State

Durable, human-readable state written by each scheduled wake. Newest at top.

## Wake 3 — 2026-10-15 21:47 (scheduled loop)
- **Unit:** GH #55 (→ #46) — recover the `MOS DATAFILE2.0` loader layout from
  the shipped engine and reconcile the parser with it.
- Observation (real evidence, read-only; Ghidra 12.1.4 via `rea`): the magic
  `"MOS DATAFILE2.0\0"` is at `MCCDyn.dll` RVA `0x1005C298` / file offset
  `0x5B698` (`.rdata`); its only xrefs are `CDataFile::Open` @`0x10032FB0` and
  `CDataFile::Create` @`0x10033F10`. `CDataFile::ReadTree_r` @`0x10032840`
  reads each node as a **0x18-byte name field + six `u32`s at `+0x18`**
  (sibling `+0x18`, child `+0x1C`, data offset `+0x20`, size `+0x24`, count
  `+0x28`, flags `+0x2C`), and the **file header is the root node record**
  (its name field *is* the magic). ADR-0413 §A had a 16-byte name + 8 fields;
  the sibling/child indices happened to be right, the split was not.
- Change:
  - `tools/mos_xmd.py`: parse `name[0x18]` + 6 fields; derive `node_start`/
    `data_start` from the root record; add `walk_linked()` (mirrors
    `ReadTree_r`) and `validate()` (root/header agreement, pointer integrity,
    linked-walk coverage, exact payload tiling) + a `--check` mode.
  - `tools/test_mos_xmd.py` (new): self-contained synthetic-container tests
    (round-trip, corrupt pointer, bad magic) — no game install needed.
  - `docs/adr/0414-mos-datafile2-loader-container.md` (new) + ADR-0413 §A
    correction note; `.github/workflows/vr-worker.yml` runs the new test.
- Evidence: `mos_xmd.py --inventory <game>\Content\Models --check` → **631/631
  `ok`, 0 failures**; `--inventory` output byte-identical old↔new for all 18
  `Characters/Riddick` models (regression-safe); `test_mos_xmd.py` PASS;
  `validate_addresses.py`, `verify_reported_fixes.py`, `scenario_tests.py
  --check` all pass; CTest 3/3 on **both** x64 and x86 (no C++ changed, so
  `build-all.ps1` was not rerun).
- Blocker: none. Next: decode the per-chunk payloads (`VERTEXFRAMES`/
  `BONEDEFORM`/`NODES`/`TRIANGLES`) using the now-pinned offsets — decompile the
  `CDF_Node` accessors + vertex/skinning decoders in `MCCDyn.dll`.
- Branch: `docs/rea-mos-loader-layout` (not pushed; `main` untouched).

## Wake 2 — 2026-10-15 21:20 (scheduled loop)
- **Unit:** GH #53 requirement 3.3 — dialogue-navigation flicker on diagonal.
- Observation: the selector latched `s_dialogueDirection` (so a held direction
  did not repeat) but `ChooseDialogueDirection` tie-breaks by `|x| > |y|`, so a
  stick resting on a diagonal (e.g. `0.62/0.60`) flips the dominant axis
  frame-to-frame and re-taps two different selections. That violates #53's
  "must not flicker at diagonal boundaries" criterion.
- Change:
  - `src/hook/dialogue_navigation.h`: added `DialogueNavigationState`,
    `UpdateDialogueNavigation` (edge-triggered + axis hysteresis,
    `kDialogueAxisSwitchMargin = 1.20f` ≈ 6° guard band), `ResetDialogueNavigation`.
  - `src/hook/camera_hook.cpp`: dialogue block now gates on menu/dialogue
    context, recentres below `deadzone * 0.55f`, and fires only the
    edge-triggered output (preserves the `verify_reported_fixes.py` invariant).
  - `tests/dialogue_navigation_tests.cpp`: added held-repeat suppression,
    diagonal no-flicker, dominance-margin boundary, recentre re-arm, and
    sub-deadzone cases.
  - `docs/adr/0410-…md`: documented the revision under "Update — GH #53".
- Evidence: `build\x64` and `build\x86` Release builds succeed (x86
  `opengl32.dll` + x64 `riddickvr-host.exe`); CTest 3/3 passed on **both** x64
  and x86; `tools/verify_reported_fixes.py` passed;
  `tools/scenario_tests.py --check` passed.
- **Coordination incident:** while this wake's edits were in progress, the
  interactive session's commit `8d36d85` (`git add -A`) swept my
  `dialogue_navigation.h` / `camera_hook.cpp` / test edits into `main` alongside
  its `.xmd` parser work. I did **not** rewrite `main` (forbidden), so the code
  half of this unit already lives on `main`; this branch carries the ADR + state
  record for it. Future wakes: stage explicitly (`git add <paths>`) to avoid the
  same sweep.
- Next: #53 3.1 (resolve the left-Y Eyeshine binding conflict explicitly) or
  3.2 (collider/target-based contextual interaction + documented proximity
  fallback); otherwise, per the updated coordination note, #55 (decode MOS
  geometry/skinning → real hand mesh).

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
