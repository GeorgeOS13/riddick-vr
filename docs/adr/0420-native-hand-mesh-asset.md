# ADR 0420: Native hand mesh asset + `.xmd` cluster→surface mapping

## Status
**Accepted** - the decoded Riddick arm/hand geometry is now baked into a
dependency-free C++ asset (`src/hook/native_hand_mesh_data.h`) that the hook
compiles in directly, and the `.xmd` per-cluster `MISCELLANEOUS` header is
decoded so each cluster resolves to a `SURFACES` index. Read-only analysis; no
game file was modified and no code is deployed.

This is the **bake/asset** step of GH #55 (advancing GH #46), following the
container layout (ADR-0414), geometry (ADR-0415), UVs (ADR-0416), skinning
(ADR-0417), skeleton (ADR-0418) and the OBJ/JSON export (ADR-0419).

## Context
ADR-0419 emitted OBJ + skin JSON for every shipped model and left two open
questions before a native first-person hand/arm path (GH #46) could be built:

1. which cluster carries which `SURFACES` material (e.g. `RIDDICK_ARMSCAR_*`),
   and
2. how to get the geometry into the game process without re-deriving the binary
   format or doing runtime file I/O.

## Finding - `CLUSTER`/`MISCELLANEOUS` is a 44-byte cluster header
Each `CLUSTER` node begins with a `MISCELLANEOUS` child whose payload is 44
bytes on **every** shipped model:

| offset | type | field |
|---|---|---|
| +0x00 | u32 | model version (`0x102`, the same value the `BONEDEFORM` reader branches on) |
| +0x04 | u32 | flags |
| +0x08 | u32 | **surface index** |
| +0x0C | u32 | reserved (0) |
| +0x10 | 24 bytes | opaque (per-cluster bounds-like floats) |

`Riddick_B05.xmd` clusters: `MISCELLANEOUS` = `02 01 00 00 | 00/03 | 00/01/02 |
00 00 00 00 | …`, i.e. a small integer that increments with the cluster.

### Aggregate evidence (all 631 shipped models, 4662 clusters)
- **`surface_index` is always `< SURFACES.count`** - 0 violations.
- **The number of distinct `surface_index` values equals `SURFACES.count` in
  every model with clusters** - 0 mismatches. (`SURFACES.count` spans 1..7:
  520 models have 1, 88 have 2, 13 have 3, ….)
- **`flags == 0` exactly when the cluster carries a non-empty `TVERTEXFRAMES`**:
  2099 textured clusters all `flags == 0`; all 2563 untextured clusters are
  non-zero (`{1: 839, 3: 1724}`). The 2099 matches the ADR-0416 UV-cluster count
  exactly, so `flags` distinguishes the visible pass from auxiliary (untextured
  / shadow) copies.
- These three invariants now run inside `mos_xmd.py --check` (631/631 `ok`).

### `Riddick_B05.xmd` cluster → surface map (decoded)
| cluster | verts | textured | surface |
|---|---|---|---|
| 0 (legs) | 372 | yes | 0 |
| 1 (left arm) | 526 | yes | **1** |
| 2 (right arm) | 344 | yes | **2** |
| 3, 4, 5 (shadow copies of 0, 1, 2) | - | no | 0, 1, 2 |
| 6 (shadow body) | 956 | no | 0 |

With the flat `SURFACES` name order for this model (base name then up to three
texture-map refs per surface) the groups resolve to `surface 0 =
PA1_INMATE_P17` (inmate outfit: legs + body), `surface 1 = RIDDICK_B03_D`
(left arm) and `surface 2 = RIDDICK_ARMSCAR_D` (right arm). **This base-name
grouping is an inference** - the record's text-layout header varies between
models, so only the *index* mapping is proven; the names come from the
`surface_names()` order (4 strings per surface in this file).

## Change
- `tools/xmd_bake.py` (new): read-only baker built on `mos_xmd`/`xmd_export`.
  `bake_mesh()` decodes a cluster; `render_header()` emits a C++ header of
  interleaved vertex records + `uint16` indices + AABB + surface id;
  `render_manifest()` emits a JSON manifest (source sha256, counts, bounds, bones
  used, surface). `--set` bakes the canonical hand set, `--model/--cluster`
  bakes ad hoc, `--verify` re-bakes and diffs against a committed header (CI-safe
  drift guard), `--cpp -` writes to stdout.
- `src/hook/native_hand_mesh_data.h` (generated, committed): **4** meshes /
  **1548** vertices / **2610** triangles (`Riddick_LeftArm`, `Riddick_RightArm`,
  `Pa2_Hand`, `Pa2_Hand_Detail`) as `inline constexpr` arrays.
- `src/hook/native_hand_mesh.h` (new, header-only): `NativeHandVertex`,
  `NativeHandMesh`, `NativeHandMeshCount()`, `NativeHandMeshAt()` (clamped),
  `FindNativeHandMesh()`.
- `tests/native_hand_mesh_tests.cpp` (new): validates the embedded asset -
  index range/coverage, unit normals, declared-vs-computed AABB, frozen
  vertex/index/surface counts (drift guard), LOD bounds agreement, name lookup.
- `tools/test_xmd_bake.py` (new): 6 headless synthetic-container tests
  (float-literal formatting, bake round-trip, no-UV fallback, manifest totals,
  `--verify` staleness, CLI error).
- `tools/mos_xmd.py`: `ClusterInfo`/`decode_cluster_info()`/`Xmd.cluster_infos()`;
  `cluster_meshes()` records `surface_index`/`cluster_flags`; `validate()` adds
  the three surface/flag invariants; summary + `--mesh` print the surface.
- `tools/xmd_export.py`: the manifest now records each cluster's `surface_index`.
- `tools/test_mos_xmd.py`: +4 cluster-info tests.
- `.github/workflows/vr-worker.yml`: runs `test_xmd_bake.py`.
- `CMakeLists.txt`: new `native_hand_mesh_asset` CTest.
- `docs/rea/reference/native_hand_mesh.json` (new) and regenerated
  `riddick_b05_export.json` / `pa2_hand_01_export.json` (now carry surfaces).

## Evidence (2026-10-16, read-only)
- `mos_xmd.py --inventory <game>/Content/Models --check` -> **631/631 `ok`, 0
  failures** with the new cluster-info invariants enabled.
- `xmd_bake.py --verify src/hook/native_hand_mesh_data.h` -> **OK: matches a
  fresh bake** (4 meshes, 1548 verts). `Riddick_B05.xmd` sha256
  `e6841e19…`, `Pa2_Hand_01.xmd` sha256 `bdd0dbd1…` (recorded in the header).
- `build-all.ps1 -Config Release` succeeds (x64 host + Win32 `opengl32.dll`);
  CTest **4/4 on x64 and x86** (`native_hand_mesh_asset` passes on both).
- `test_mos_xmd.py`, `test_xmd_export.py`, `test_xmd_bake.py`,
  `validate_addresses.py`, `verify_hook_init.py`, `verify_reported_fixes.py`,
  `scenario_tests.py --check` all PASS.

## Unknowns / follow-ups
- **Base-name grouping** of `SURFACES` (which strings belong to which surface)
  is inferred from record order, not parsed; the record text header varies
  between models (a `02 02 21 …` prefix is not universal). Only the numeric
  index mapping is proven.
- The `MISCELLANEOUS` +0x10.. block is not decoded (looks bounds-like but does
  not match the `VERTEXFRAMES` AABB).
- The baked asset is **bind pose** (`.xmd` stores no rotations) and is not yet
  consumed by `VRHandRenderer`. **Next:** add the native draw path behind a
  config flag (rigid first, then skinned once bone rotations from `KEYS` are
  decoded), keeping the procedural overlay as the fallback.
