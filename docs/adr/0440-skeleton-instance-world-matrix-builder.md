# ADR 0440: the live per-bone matrix array is raw FK world matrices (no bind-inverse)

## Status
**Accepted (read-only RE finding)** — answers the "Next probe" left by ADR-0430
(GH #60): the builder of the live per-bone matrix array. Verified against the
MXR.dll disassembly. No game file was modified; no code was deployed; nothing was
executed.

This continues the `#59 → #60 → #46/#58` chain:
ADR-0419 → 0423 → 0424 → 0425 → 0426 → 0428 → 0429 → 0430 → **0440**.

## Context
ADR-0428/0429/0430 pinned *who consumes* the live per-bone 3×4 matrix array
(`Cluster_TransformBones_V_N_TgU_TgV` = `FUN_100254e0`, via the provider
`FUN_100238a0`/`FUN_10022f90`) and *where it is reached from* in the model draw
(`*(*(modelInstance+0x94)+0x14)` list, override `*(arg6+0x3c)`). The open
question (GH #60 step 1) was: **does its builder compose `world·bind_inverse`, or
does it store raw `world`?**

## Finding (MXR.dll sha256 `06424f55…`, read-only, Ghidra 12.1.4)

### 1. The builder is `CXR_SkeletonInstance` forward kinematics
Three functions form the chain, all reached from the skeleton instance:

- **`FUN_10021650`** — a **virtual** method. Its code pointer is stored in
  `.rdata` (file `0x14354C`) immediately before the string block
  `"Obsolete function"` / `".\XRSkeleton.cpp"` / `"EvalAnim"` / `"ATTACHPOINTS"` /
  `"NODEINDICES"` / `"NODES"`, so the owning class is **`CXR_SkeletonInstance`**
  (RTTI `.?AVCXR_SkeletonInstance@@` @ `0x1016a4c0`, in `XRSkeleton.cpp`).
  Signature: `this, int *pose, uint count, void *poseDesc, float *root4x4,
  uint flags`.
- **`FUN_10020b50`** — evaluates the per-node **local pose** (quaternion pool →
  `poseDesc+0x24`, translation pool → `poseDesc+0x2c`), called once from
  `FUN_10021650`.
- **`FUN_1001f340`** — walks the skeleton node tree and composes the per-node
  **world** 3×4 matrices, called once from `FUN_10021650`.

`FUN_10021650` first validates the caller-supplied **root 4×4** (16 floats,
every element finite) and pre-multiplies it into the pose quaternions/translations
(`flags`-selected rotate+translate / translate-only / position-only modes), then
drives the FK.

### 2. The composition is `world[node] = world[parent] · local[node]` — no bind-inverse
`FUN_1001f340` traverses the tree from the root via the skeleton's child table
(`this+0xc` = child-index array; `this+0x14` = 0x20-byte node records:
position `[0..2]` @`+0x00`, first-child index `short` @`+0x0e`, child count `byte`
@`+0x10`, connect/parent `short` @`+0x12`, rotation slot `short` @`+0x1c`,
movement slot `short` @`+0x1e`). Per node it:

- builds the **local** 3×4 matrix `local[node] = quat(rotationPool[rotationSlot])
  → 3×4` (`FUN_1001dc30`) with the animated translation
  `translationPool[movementSlot]` (negated) in the translation column, written to
  `*(poseDesc+0x34) + node*0x30`;
- composes **`world[node] = parentWorld · local[node]`** with
  **`FUN_1001db60`** and writes it to `*(poseDesc+0x30) + node*0x30`.

`FUN_1001db60` is a plain 3×4 concat, verified algebraically:
`out = parent · local` (rotation block = 3×3 product, translation =
`parent.r · local.t + parent.t`).

**No bind / reference / inverse matrix is read or composed anywhere in
`FUN_10021650` / `FUN_10020b50` / `FUN_1001f340`.** The only non-trivial extra is
a per-node world-position accumulator (`local_3c/38/34`) that uses the node's
`NODES` bind position and the connected node (`node+0x12`) plus a per-node weight
(`poseDesc+0x1c`) — a *bone-connect* correction for the FK translation, not a
skin bind-inverse.

### 3. The draw consumes a per-node 0x30-byte matrix list on the pose descriptor
Re-reading `FUN_1002dcc0`: `poseDesc = modelInstance[0x94]` (override
`*(arg6+0x3c)`); the per-bone matrix is
`*(*(poseDesc+0x14)+0x18) + bone*0x30`, where `bone` = the per-cluster bone (node)
index from the cluster→bone `ushort` array; its 12 floats are copied into a 4×4
(row padding 0, `[0xf]=1`). So the consumer indexes a **per-node** matrix list on
the pose descriptor — consistent with the FK output, but the `poseDesc+0x14` list
object is **not** the `poseDesc+0x30`/`+0x34` array pointer (see *not claimed*).

### 4. The shipped skinned geometry is authored in MODEL space
`tools/mos_xmd.py --mesh` on `Riddick_B05` (7 skinned clusters): every cluster's
reference box spans model units that match the `NODES`/live extents (arm clusters
`y −25.4..6.4`, `z 32.8..52.5`), i.e. the decoded geometry is **not** clustered
near a bone origin. So the *file* geometry is in model (bind) space.

## Decision / consequence
- **Answer to #60 step 1: the live per-bone matrix builder stores raw FK `world`
  matrices and composes NO bind-inverse.** `world = parentWorld · local` from the
  `NODES` bind positions + the animated quaternion/translation pools.
- Consequence for a skinned native hand (#46/#58): the honest offline recipe is
  now pinned to **forward kinematics from `NODES` + a `.xsa` pose** (the same
  parent-walk), not "find a per-cluster bind transform". A `world·bind_inverse`
  form is only needed if the geometry fed to the blend is model-space (see the
  tension in §4 and *not claimed* below).
- Consequence for #60: the runtime read (#34) still stands as the faithful route,
  but the matrix semantics are now known (raw world), which is what a runtime
  reader must feed into the ADR-0428 blend.

### Not claimed
- **Not claimed** that the `poseDesc+0x14` list the draw reads *aliases*
  `poseDesc+0x30` (world) or `poseDesc+0x34` (local), or is a distinct skin array.
  §4 (model-space geometry) and §2 (no bind-inverse) are only consistent if
  either (a) the engine rebases the mesh vertex buffer to bone-local at load, or
  (b) a bind-inverse step exists between the FK output and the consumed list. This
  is the corrected **next probe**.
- **Not claimed** that `local_3c/38/34` / the `poseDesc+0x1c` weight fully
  reconstruct the FK translation; the decompiler merged several stack floats into
  one 3×4 buffer, so the exact translation algebra was not exhaustively checked.
- **Not claimed** that `CXR_SkeletonInstance` is the owner of the *draw* arg
  `modelInstance[0x94]`; only that the FK *builder* `this` is that class (vtable
  evidence). The pose descriptor (`modelInstance[0x94]`) is a distinct object.
- The exact vtable *base* of `CXR_SkeletonInstance` is not resolved (RTTI/COL
  pointers are interleaved in the region); only the slot holding `FUN_10021650` is
  proven.
- The SSE variants were not analysed; findings are for `System/Win32_x86`.

## Change
- `docs/rea/reference/MXR_SkeletonInstance_WorldMatrices.txt` (new) — the
  pseudocode, the concat algebra, the vtable/class evidence, the draw consumer,
  and the `Riddick_B05` reference boxes.
- `docs/rea/mos-xmd-format.md`: the provider "Next probe" is answered and
  re-pointed at the aliasing / bind-inverse step.

## Evidence (2026-10-16, read-only)
- `rea search MXR.dll "Skeleton" --kind procedures` → no named procedures (PDB not
  loadable); `--kind strings` → the RTTI set (`0x1016a4c0` = `CXR_SkeletonInstance`)
  and `".\XRSkeleton.cpp"` (`tools/autoloop/logs/wake33_search_skeleton_*.json`).
- `rea function MXR.dll` on `0x10021650` / `0x10020b50` / `0x1001f340` /
  `0x1001db60` (`tools/autoloop/logs/wake33_func_*.json|.pseudo.txt`).
- Raw byte scan of `MXR.dll` for the little-endian pointer of `0x10021650`
  (`50 16 02 10`) → single hit at file `0x14354C`, directly before the
  `".\XRSkeleton.cpp"` / `"EvalAnim"` string block (the vtable slot).
- `rea xrefs MXR.dll 0x10144764` → the XRSkeleton.cpp-bearing functions
  (`0x1001f73f`, `0x1001fcc0`, `0x1001fe51`, `0x1001fffd`, `0x10020056`,
  `0x10021a10`, `0x10021b02`).
- `python tools/mos_xmd.py <Riddick_B05.xmd> --mesh|--skeleton --json`
  (`tools/autoloop/logs/wake33_b05_mesh.json`, `wake33_b05_skeleton.json`).
- No C++/asset changed → `build-all.ps1`/CTest not rerun. Python verifiers:
  `test_mos_xmd.py`, `test_xmd_export.py`, `test_xmd_bake.py`, `test_xmd_pose.py`,
  `test_xsa_anim.py`, `test_mos_xtc.py`, `test_xtc_bake.py`,
  `validate_addresses.py`, `verify_hook_init.py`, `verify_reported_fixes.py`,
  `scenario_tests.py --check` all PASS.

## Files
- `docs/adr/0440-skeleton-instance-world-matrix-builder.md` (this file),
  `docs/rea/reference/MXR_SkeletonInstance_WorldMatrices.txt`,
  `docs/rea/mos-xmd-format.md`.
- cross-refs: ADR-0428/0429/0430, GH #34/#46/#55/#58/#59/#60.
