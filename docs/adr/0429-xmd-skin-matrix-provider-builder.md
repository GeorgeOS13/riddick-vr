# ADR 0429: the `.xmd` skin matrix provider is a palette selector, not a bind-inverse builder

## Status
**Accepted (read-only finding)** — answers the last "Next probe" left open by
ADR-0428. The draw-side matrix *provider* is a transient `0x10`-byte object built
per skinned cluster draw by **`FUN_100238a0`**; it holds **no** bind/reference/
inverse matrix. It only selects a matrix out of a **live** per-bone array supplied
by the model instance, remapping the influence's `bone` byte through the mesh's
own `BONEMATRIXMAP` palette. No game file was modified and no code was deployed.

This extends the `#59` chain: ADR-0419 → ADR-0423 → ADR-0424 → ADR-0425 →
ADR-0426 → ADR-0428 (draw-side blend) → **ADR-0429** (provider builder).

## Context
ADR-0428 decompiled `CXR_Model_TriangleMesh::Cluster_TransformBones_V_N_TgU_TgV`
(`FUN_100254e0`) and showed it transforms each vertex by a weighted sum of 3×4
matrices fetched with `FUN_10022f90(provider, bone)` from a second argument. It
left one question: **who builds that provider's matrix array, and does the build
multiply in a bind-inverse (`world · bind_inverse`) or store raw `world`?** This
wake decompiled the builder and the getter's data source.

## Finding (MXR.dll sha256 `06424f55…`, read-only, Ghidra 12.1.4)

### 1. The provider builder — `FUN_100238a0` (`0x100238a0`)
Called from the model draw method `FUN_1002dcc0` on the skinned-cluster path
(`LAB_1002f3e0`, taken when `mesh[0x2b] & 0x4000` = "has bone deform"):
`uVar5 = FUN_100238a0(pfVar6, local_84, local_94, 0x1016a7d0)`. It allocates a
**`0x10`-byte** object and stores it into the render descriptor at `desc+0x0C`
(`desc` = the global `0x1016a7d0`, `desc+0x0C` = `DAT_1016a7dc`):

| provider field | value |
|---|---|
| `+0x00` `float* matrices` | **`param_2`** — the live per-bone 3×4 array supplied by the caller |
| `+0x04` `u8* palette` | **`FUN_1007fa90(mesh)`** = `(mesh+0x24) + 4` — the mesh's `BONEMATRIXMAP` **palette bytes**; **`NULL`** when the mesh has none (`FUN_1007eda0(mesh) == 0`) or `param_2 == 0` (then `bone` indexes directly) |
| `+0x08` `u32 count` | `FUN_1007eda0(mesh)` = the palette length (`*(u32*)(mesh+0x24)`); in the no-palette branch `min(*(int*)(mesh+0x2c), param_3)` |
| `+0x0C` | `0` |

The provider is exactly the `{float* matrices; u8* palette}` object whose getter
`FUN_10022f90` ADR-0428 decoded. **`FUN_100238a0` reads nothing but the mesh's
`+0x24` block and the caller's arguments** — it computes no transform and reads no
bind/reference/inverse matrix.

### 2. `mesh+0x24` is the file's `BONEMATRIXMAP`, and its shape is `{u32 n; u8[n]}`
- `FUN_1007fa90(mesh)` returns `*(int*)(mesh+0x24) + 4` (the byte array after a
  leading `u32`); `FUN_1007eda0(mesh)` returns `**(u32**)(mesh+0x24)` (that `u32`).
- Both first walk `mesh+0x28` up the mesh-parent chain (`FUN_10079ad0`) to the
  root mesh before reading `+0x24`; a lazy-`Deform` guard (`mesh+0xac & 1/0x4000/
  0x8000`) may call `FUN_1007ecd0` first.
- This is the same block `tools/mos_xmd.py` decodes as `matrix_bones` and that
  `CTriangleMeshCore::Read` stores at mesh `+0x24` (ADR-0426); so the runtime
  palette **is** the file's `matrix_bones`, one byte per influence index.
- (`FUN_1007fa40(mesh)` in ADR-0428 = `*(int*)(mesh+0x20) + 0x18` = the influence
  pool; the `+0x20`/`+0x24`/`+0x28` roles line up with ADR-0426.)

### 3. The provider's matrices come from the model instance, not the mesh
Inside `FUN_1002dcc0`, before the cluster loop:
```
local_a0 = param_1[0x2c];                     // param_1 = draw arg #1 (model object)
iVar15   = *(int *)(local_a0 + 0x14);
local_84 = (int *)*(int *)(iVar15 + 0x18);    // -> live per-bone matrix array
local_94 = *(int *)(iVar15 + 4);              // -> its entry count
```
So `matrices` is a **field (`+0x18`) of the object at `*(model+0x2c)+0x14`** and is
walked once per draw, then handed to every skinned cluster. It is not derived from
the mesh, the cluster, `MISCELLANEOUS` or `BONEMATRIXMAP`.

### 4. The getter (unchanged from ADR-0428)
`FUN_10022f90(provider, bone) = (palette ? palette[bone] : bone) * 0x30 + matrices`.
No transform arithmetic. The vertex blend `Σ_k w_k·M(palette[bone_k])` is applied
entirely inside `FUN_100254e0`.

## Decision / consequence
- **The draw-side provider builder does not multiply in a bind-inverse.** Every
  matrix on the `.xmd` draw path is read from a **live** array owned by the model
  instance object (`*(model+0x2c)+0x14`, field `+0x18`). The mesh contributes only
  geometry, influence records, the influence pool and the `BONEMATRIXMAP` palette.
- Therefore GH #59's "per-cluster reference/bind-inverse transform" is now
  resolved **upstream**: if it exists at all it is composed when that live array is
  built, which is *not* the `.xmd` draw path and not `FUN_100238a0`/`FUN_10022f90`.
- **For the native hand (#46/#58):** a *correct* skinned draw cannot be derived
  from `.xmd` data alone. The two honest options remain:
  1. **read the live matrix array at runtime** (the model-instance field
     `*(model+0x2c)+0x14 → +0x18`) via engine calls — GH #34; or
  2. keep the **rigid** draw (ADR-0427) until a pose source (bind-inverse ×
     `.xsa` pose, ADR-0422) is reconstructed offline.
- It also explains ADR-0428's apparent paradox (geometry ≠ `NODES` bind yet the
  blend has no bind term): whatever makes the result correct is already in the
  **live** matrices, so a naive `NODES`-derived `world·bind_inverse` would double-
  transform.

### Not claimed
- Not claimed which class the live matrix array / its owner object is; only the
  field chain `param_1[0x2c] → +0x14 → {+0x18 matrices, +0x04 count}`.
- Not claimed whether the live matrices are `world` or `world·bind_inverse`, nor
  where they are written; only that `FUN_100238a0` and `FUN_10022f90` do **not**
  compose any bind-inverse and read no such matrix from the mesh.
- Not claimed the `mesh+0x28` chain walk is a pose/LOD/warp link (ADR-0426 read
  `+0x28` as `matrix_map_id`); this ADR only records that the accessors traverse it
  before reading `+0x24`.
- The GPU/`SSE` variants were not analysed; findings are for `System/Win32_x86`.

## Change
- `docs/rea/reference/MXR_SkinMatrixProvider.txt` (new): pseudocode of
  `FUN_100238a0`, the two `mesh+0x24` accessors `FUN_1007fa90`/`FUN_1007eda0`, the
  unchanged `FUN_10022f90`, and the `FUN_1002dcc0` call-site/field chain.
- `docs/rea/mos-xmd-format.md`: the "Draw-side skinning" section gains the
  provider-builder result and its Next step is retargeted at the live array; the
  `BONEDEFORM` section notes `mesh+0x24 = {u32 n; u8[n]}`.

## Evidence (2026-10-16, read-only)
- `rea xrefs MXR.dll 0x1016a7dc` → `[0x100245c5, 0x1002f4f9, 0x1002f50e, 0x1002f51d,
  0x1002f532]` (evidence `ev_f48b2972…`); `0x1016a7d0` → `[…, 0x1002d9e3, 0x1002f3e2,
  0x1002f40d, 0x1002f442, 0x101403e0]` (`ev_1dc453ec…`).
- `rea function MXR.dll 0x100238a0` → `FUN_100238a0` (`ev_b5ceb499…`): the `0x10`-byte
  provider builder above.
- `rea function MXR.dll 0x1007fa90` → `FUN_1007fa90` (`ev_c9ad07cc…`) = `mesh+0x24 + 4`.
- `rea function MXR.dll 0x1007eda0` → `FUN_1007eda0` (`ev_c2f1d5c9…`) = `*(u32*)(mesh+0x24)`.
- `rea function MXR.dll 0x1007fa40` → `FUN_1007fa40` (`ev_013f963f…`) = `*(mesh+0x20)+0x18`.
- `rea function MXR.dll 0x1002f4f9` → `FUN_1002dcc0` (`ev_06784cb6…`, 47.7 k chars):
  the `LAB_1002f3e0` skinned path, the `FUN_100238a0` call and the `local_84` chain.
- `rea function MXR.dll 0x1002d890` → `FUN_1002d890` (`ev_a99f99f5…`): the
  `CXR_Model_TriangleMesh` constructor (RTTI `.?AVCXR_Model_TriangleMesh@@`) that
  zeroes `0x1016a7d0` via `FUN_10103a50` (`ev_a7e5133b…`, `0x28`-byte clear).
- `rea search MXR.dll Skeleton` → `CXR_SkeletonInstance` (RTTI `0x10142fc8` /
  `.?AVCXR_SkeletonInstance@@ 0x1016a4c0`) — candidate owner of the live array, not
  confirmed here (`ev_d6ca4d4c…`).
- No C++/asset changed → `build-all.ps1`/CTest not rerun. Python verifiers:
  `test_mos_xmd.py`, `test_xmd_export.py`, `test_xmd_bake.py`, `test_xmd_pose.py`,
  `test_xsa_anim.py`, `validate_addresses.py`, `verify_hook_init.py`,
  `verify_reported_fixes.py`, `scenario_tests.py --check` all PASS.

## Files
- `docs/rea/reference/MXR_SkinMatrixProvider.txt`, `docs/adr/0429-…md`,
  `docs/rea/mos-xmd-format.md`
- cross-refs: ADR-0419/0422/0423/0424/0425/0426/0427/0428, GH #34/#46/#55/#58/#59
