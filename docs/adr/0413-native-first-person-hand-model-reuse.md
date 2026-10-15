# ADR 0413: Native First-Person Hand/Arm Model Reuse (Investigation)

## Status
**Proposed — investigation only. No native path is implemented or tested yet.**

The currently rendered hands remain the procedural OpenGL overlay in
`src/hook/vr_hand_renderer.cpp` (`VRHandRenderer`). They are **not** Riddick's
in-game model or skeleton. This ADR records what is and is not reachable, so the
procedural renderer is not mistaken for a native result.

## Context
1. ADR-0406 already acknowledges the hands are a bespoke procedural renderer,
   not engine geometry. Users still see "Roblox-like" hands and want the real
   Riddick hands (GH #46).
2. The mod renders **monoscopically**: `StereoRenderer::OnSwapBuffers` captures
   the single game backbuffer and blits it into two eye FBOs with a half-IPD
   shift (`src/hook/stereo_renderer.cpp:811-912`). No native dual-view render.
3. `src/hook` intercepts only `wgl*`, `glViewport`, `glMatrixMode`,
   `glLoadMatrixf`, `glFrustum`, `glOrtho`, `glClear`, `glBindFramebuffer`, and
   an IAT hook of `GDI32!SwapBuffers` in `RndrGL.dll`. `glUniformMatrix4fv` is
   **not** hooked (contrary to `src/hook/README.md:9`). Matrix-concatenation
   calls (`glPush/PopMatrix`, `glTranslatef`, `glRotatef`, `glMultMatrixf`) are
   pure forwarders, so a game "viewmodel" transform submitted through them is
   currently invisible to the hook.
4. `CameraHook::StabilizeViewMatrix` (`src/hook/camera_hook.cpp:228`) and
   `OpenGLProxy::GetLastViewMatrix` are dead/unwired.
5. **Target mismatch:** `config/addresses.json` and the `docs/rea-msystem`
   recon target the 2009 *Assault on Dark Athena* `DarkAthena.exe` + 5.1 MB
   `MSystem.dll`. The machine's installed copy is the **2004 *Escape from
   Butcher Bay*** (`Riddick.exe` + `System/Win32_x86/SbzEngine.exe`, ~2.8 MB
   `MSystem.dll`, `RndrGL.dll` 513 KB). Any engine-offset work must pin the
   build it targets (GH #55).

## Findings (evidence, 2026-10-15, read-only)
### A. The real models are present and machine-readable
- Riddick character/weapon meshes live under
  `Content/Models/**/*.xmd`. Files are Starbreeze **`MOS DATAFILE2.0`**
  containers (631 `.xmd` under `Content/Models`).
- Container layout recovered and validated by dereference (tool
  `tools/mos_xmd.py`):
  - header `0x30`: `magic[16]`, `+0x18` node-table start (`0x30`),
    `+0x20` data-region start.
  - node table: contiguous `0x30`-byte records `name[16]` + `u32[8]`,
    where `[2]=next sibling`, `[3]=first child`, `[4]=data offset`,
    `[5]=data size`, `[6]=count`, `[7]=flags`.
  - payloads tile the data region contiguously; offsets/sizes verified.
  - **Corrected by ADR-0414** (loader decompilation): the record is
    `name[0x18]` + `u32[6]` at `+0x18` (sibling `+0x18`, child `+0x1C`,
    data offset `+0x20`, size `+0x24`, count `+0x28`, flags `+0x2C`), and the
    file header *is* the root node record. The sibling/child/data indices used
    here were already right; only the 16-vs-24-byte name split was wrong.
- Chunk inventory per model: `SURFACES`, `BOUND`, `CLUSTERS` → per-cluster
  `VERTEXFRAMES` (positions), `TVERTEXFRAMES` (UVs, flags `0x201`),
  `BONEDEFORM` (skinning), `TRIANGLES` (`u32 count` + `u16` indices — verified:
  declared count == node count), `PRIMITIVES`, `EDGES`/`EDGETRIS`/`TRIEDGES`,
  and `SKELETON` → `NODES` (bone count; e.g. Riddick_B03 = 74, B05 = 70,
  34 bytes/node) + `NODEINDICES` + `KEYS`.
- `KEYS` holds build provenance only (e.g. `MODEL=Characters\Riddick\
  Riddick_B05.xmd`, `XTXPATH=S:\PB\Models\...`), **not** readable bone names.
- Hand/arm-relevant assets confirmed: `Riddick_B05.xmd` carries
  `RIDDICK_ARMSCAR_D/S/N` surfaces; `Content/Models/Characters/Pa2/
  Pa2_Hand_01.xmd` is a standalone hand mesh.

### B. What is NOT yet established
- The exact `VERTEXFRAMES`/`BONEDEFORM`/`NODES` byte layouts (positions are
  byte-packed; likely per-frame deltas). Needs the loader code.
- Bone **names** (positions of hand/forearm bones) — `NODES` stores no strings;
  may need the animation groups (`.xsa/.xag/.xrg`) or the loader.
- Whether the engine's true first-person "viewmodel" is the body arms, a
  separate model, or per-weapon arms. No dedicated `*_FP*` model was found in
  `Content/Models`.
- Whether engine-side control of its own viewmodel is reachable — blocked on a
  decompiler (see below).

### C. Tooling
- REA 6.5.0 CLI is installed; **no** Ghidra/IDA/Hopper was configured at start.
  Ghidra 12.1.4 (matching REA's expected 12.1.x) is being provisioned with the
  existing Java 21 runtime so `rea` can decompile the loader/viewmodel code.
  Previously only static PE/string/format analysis was available.

## Options
- **A. Extract + re-render the real mesh (recommended first target).** Decode
  `VERTEXFRAMES`/`TVERTEXFRAMES`/`TRIANGLES`/`BONEDEFORM` + `NODES`, extract the
  hand/forearm surface from `Riddick_B05`/`Pa2_Hand_01`, skin it to our
  controller-driven skeleton, and render it in `VRHandRenderer` using the real
  diffuse texture. Fully in our control; no engine-offset risk; directly gives
  the user the actual Riddick hand art. Cost: format decode + skinned draw.
- **B. Drive the engine's own viewmodel.** Force `noviewmodel` off and override
  the viewmodel matrix at `OpenGLProxy::Hook_glLoadMatrixf`. Highest fidelity
  (real skeleton + materials + animation) but blocked until (i) the viewmodel
  transform path is located in the engine and (ii) it is reachable given the
  un-hooked concatenation calls. Requires decompiler work.
- **C. Keep procedural overlay.** Fallback only; retained until A or B is
  tested in-headset.

## Decision
1. Keep the procedural `VRHandRenderer` as the shipping fallback; it must not be
   described as native.
2. Pursue **Option A** in parallel with Ghidra-backed reconnaissance for
   **Option B**, behind a config flag, once a real mesh path exists.
3. Any native path must be tested in-headset before GH #46 is closed.

## Consequences
- The asset path is proven accessible read-only; no game files are modified.
- `tools/mos_xmd.py` provides a dependency-free, read-only inventory/parse used
  by tests and by the autonomous REA loop.
- Engine-side work must pin the exact executable build (GH #55).

## Open questions / follow-ups
- Decode `VERTEXFRAMES`/`BONEDEFORM`/`NODES` layouts (Ghidra xref on the
  `MOS DATAFILE2.0` loader in `MXR.dll`/`MSystem.dll`).
- Locate first-person viewmodel transform in `RndrGL.dll`/`SbzEngine.exe`.
- Add `Hook_glUniformMatrix4fv`/matrix-concat observation if Option B proceeds.
