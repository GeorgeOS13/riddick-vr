# ADR 0441: the per-bone skin matrices are bind-relative deltas (identity at bind), applied to model-space geometry

## Status
**Accepted (read-only RE finding)** — resolves both halves of the "not claimed #1"
left by ADR-0440 (GH #60 step 1). No game file was modified; no code was deployed;
nothing was executed.

Continues the `#59 → #60 → #46/#58` chain:
ADR-0419 → 0423 → 0424 → 0425 → 0426 → 0428 → 0429 → 0430 → 0440 → **0441**.

## Context
ADR-0440 proved the live per-bone "world" matrix array is produced by
`CXR_SkeletonInstance` forward kinematics (`FUN_10021650` → `FUN_10020b50` →
`FUN_1001f340`) with **no bind-inverse in the chain**, and left two things open:

1. whether the list the draw consumes (`poseDesc+0x14`) *aliases* the FK output
   arrays (`poseDesc+0x30` world / `poseDesc+0x34` local); and
2. since the shipped skinned geometry is authored in **model space**
   (`Riddick_B05` arm clusters `y −25.4..6.4`, `z 32.8..52.5`), how model-space
   geometry can be correct with a bind-inverse-free chain — the ADR framed this as
   "either (a) a load-time vertex rebase, or (b) a bind-inverse step between the FK
   output and the consumed list".

## Finding (MXR.dll sha256 `06424f55…`, read-only, Ghidra 12.1.4)

### 1. The aliasing is resolved: the consumed list IS the FK world output
`FUN_1001de10(this = poseDesc, n)` — the array (re)sizer called by `FUN_10021650`
when the node count exceeds the descriptor's current length — resizes the
descriptor's three embedded `CListVectorCore` members and then **caches their data
pointers**:

```
this+0x30 = *(*(this+0x14) + 0x18);   // "world"  <- data of the +0x14 list
this+0x34 = *(*(this+0x0c) + 0x18);   // "local"  <- data of the +0x0c list
```

Both arrays are initialised to the identity 3×4 (12 floats) whenever the length
changes; the third list (the per-node weight at `+0x1c`) is zeroed. The draw
`FUN_1002dcc0` consumes exactly `*(*(poseDesc+0x14) + 0x18) + bone*0x30`.

Therefore the per-bone matrix the draw hands to the skinning is **the FK world
array itself** (`*(poseDesc+0x30)`) — bit-identical, same object.

### 2. There is no bind-inverse in the skin path either
The skinning (`FUN_100254e0`) fetches each 3×4 with `FUN_10022f90(provider, bone)`:

```c
int FUN_10022f90(void *this, int bone) {
  if (*(int *)((int)this + 4) != 0)                      // palette remap (u8/bone)
    return *(u8 *)(*(int *)((int)this + 4) + bone) * 0x30 + *(int *)this;
  return bone * 0x30 + *(int *)this;                     // direct 0x30-byte fetch
}
```

— a **raw 0x30-byte fetch with no arithmetic**. The `provider` is the
`FUN_100238a0` bundle `{+0x00 = the live world array, +0x04 = palette, +0x08 n}`.
So option **(b) is dead**: there is no bind-inverse step between the FK output and
the consumed list.

### 3. The consumed matrix is applied as a plain affine, no bind subtraction
The single-influence branch of `FUN_100254e0` transforms the position with

```
out = v·R + T          (T = m[9], m[10], m[0xb] added directly)
```

and the multi-influence branch is the weight-sum of the same affine maps. There is
no `v − bindpos` and no bind matrix read anywhere in the skinning.

### 4. The "world" matrices are bind-relative deltas — identity at bind
`FUN_1001f340` overwrites the local matrix's translation before the concat (the
"bone-connect correction"). For the zero-weight / unconnected branch the algebra is
exactly:

```
fVar    = (M_local · bindpos) − bindpos          // per axis (m[9..0xb] = −t)
local.t = −fVar = bindpos − R·bindpos + t
```

i.e. the local affine is `v ↦ R·(v − bindpos) + bindpos + t` — "rotate about the
bone's bind position, then add the animated translation". At bind (identity
quaternions, `t = 0`) every local reduces to identity, and the root's parent is the
identity (`local_8b4[0] = &local_ac = {1,0,0,0, 0,1,0,0, 0,0,1,0}`); hence every
**world reduces to identity**.

So `world(node)` is a **delta from the bind pose**, not an absolute bone transform,
and it is designed to be applied to the **bind-pose (file, model-space) geometry
directly**. That is exactly why the model-space `Riddick_B05` refboxes render
correctly in the live game with no rebase and no bind-inverse.

### 5. The vertex accessor does not transform
`FUN_1007ee20(mesh, variant)` returns `*(listobj + 0x18)` — the raw per-variant
position buffer (streamed from the MOS file via `FUN_1007c6c0` when not resident).
It performs no coordinate change.

## Decision / consequence
- **ADR-0440 "not claimed #1" answered:** `poseDesc+0x14` *does* alias
  `poseDesc+0x30`; the draw/skin consumes the FK output directly.
- **ADR-0440's tension is dissolved — the (a)/(b) dichotomy was false.** There is
  neither a load-time vertex rebase nor a bind-inverse step. The engine's "world"
  matrices are bind-relative **deltas** (identity at the bind pose) applied straight
  to the model-space mesh.
- Consequence for a skinned native hand (#46/#58/#60): the offline recipe is
  **FK from `NODES` + a `.xsa` pose, producing per-bone bind→animated deltas, then
  applied to the model-space `.xmd` geometry** — the same parent-walk the engine
  does, with **no** per-cluster bind transform to find (this also confirms the
  negative in ADR-0424/0428/0429: there is no mesh-side reference matrix).
- Consequence for the current rigid native draw (#64/ADR-0436): because the delta
  is identity at bind, the file mesh **is** the rest-pose model. Drawing it as-is
  (the current rigid path) is a legitimate baseline; skinning is what the animated
  deltas add.

### Not claimed
- The exact algebra of the **weighted/connected** local branch (`connect != −1`,
  weight `!= 0`) was not expanded. It applies a weight-blended **bind-offset**
  correction; it is still a bind-relative correction, not a bind-inverse. The
  identity-at-bind reduction was verified for the zero-weight / unconnected branch.
- The bind→animated delta was derived by algebra from the disassembly, **not
  executed**; no runtime read was taken (that remains GH #34).
- The live vertex buffer was not read, so "the runtime buffer is the file's
  model-space data" is inferred from the file decoding (§ `Riddick_B05` refboxes)
  plus "the accessor does not transform" — strong, but not a memory capture.
- The SSE variants were not analysed; findings are for `System/Win32_x86`.

## Change
- `docs/rea/reference/MXR_SkeletonInstance_WorldMatrices.txt`: a wake-34 addendum
  with the aliasing resolution, the accessor, the skinning affine and the
  bind-relative-delta algebra.
- `docs/rea/mos-xmd-format.md`: the ADR-0440 "Next probe" is answered and closed.
- New `tools/autoloop/extract_pseudo.py`: focus a `rea function --format json`
  capture down to its Ghidra pseudocode (used to read the captures below).

## Evidence (2026-10-16, read-only)
- `rea function MXR.dll` on `0x1001de10` (`wake34_func_1001de10.json|.pseudo.txt`),
  `0x100254e0` (`wake34_func_100254e0.*`), `0x10022f90`
  (`wake34_func_10022f90.*`), `0x1007ee20` (`wake34_func_1007ee20.*`); plus the
  wake-33 `FUN_1001f340` capture. Raw captures are in `tools/autoloop/logs/wake34_*`.
- Provider: Ghidra 12.1.4 via `rea` (MXR.dll, sha256 `06424f55…`).
- No C++/asset changed → `build-all.ps1`/CTest not rerun. Python verifiers +
  `scenario_tests.py --check` PASS (see STATE.md wake 34).

## Files
- `docs/adr/0441-fk-world-matrices-are-bind-relative-deltas.md` (this file),
  `docs/rea/reference/MXR_SkeletonInstance_WorldMatrices.txt`,
  `docs/rea/mos-xmd-format.md`, `tools/autoloop/extract_pseudo.py`.
- cross-refs: ADR-0424/0428/0429/0430/0440, GH #34/#46/#55/#58/#59/#60.
