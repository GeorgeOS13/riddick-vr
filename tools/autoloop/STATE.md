# RiddickVR Autoloop — State

Durable, human-readable state written by each scheduled wake. Newest at top.

## Wake 8 — 2026-10-16 00:15 (scheduled loop)
- **Unit:** GH #55 → #46 — correct/extend the just-committed `.xmd` `SKELETON`
  decode: pin the `+0x0E` node field and the real meaning of `NODEINDICES`.
- Coordination: while this wake was doing independent RE, a **second concurrent
  writer** (extra `opencode` sessions were live) implemented the skeleton step
  and committed it as `2a5430d` (Wake 7). Working tree was clean by the time I
  went to commit, so this wake **builds on** that commit instead of duplicating
  it (no clobbering; explicit `git add <paths>` only).
- Observation (real evidence, read-only; Ghidra 12.1.4 via `rea`): the
  **load-time hierarchy walker `FUN_1001e040`** (called by `FUN_1001f2e0`, in
  turn called at the end of the skeleton reader `FUN_100219d0`) recurses over the
  tree. For node `i` it reads `child_count = *(u8*)(node + 0x10)` and
  `child_start = *(i16*)(node + 0x0E)` and takes each child as
  `*(u16*)(NODEINDICES_base + (child_start + k) * 2)`, recursing on it. It never
  reads the `parent` field (`+0x12`). Therefore Wake 7's "`+0x0E` index
  (uninterpreted)" is really **`child_start`**, and `NODEINDICES` is the
  **flattened child table** (`node.children == NODEINDICES[child_start :
  child_start + child_count]`), not merely the set of used non-root nodes.
- Change:
  - `tools/mos_xmd.py`: `SkeletonNode.child_start` replaces `unknown_e`;
    `Skeleton.children_of()` now reads the `NODEINDICES` table exactly like
    `FUN_1001e040`; added `child_ranges_tile()` and `derived_parents()`;
    `child_count_matches()` now cross-checks the table against the `parent`
    field; `validate()` adds child-range bounds, exact tiling,
    `sum(child_count) == len(NODEINDICES)`, and table-vs-parent-field agreement;
    padding nodes must also have `child_count == 0`; `geometry_summary()` reports
    `skeleton_child_table_ok`; `--skeleton` prints `child_start`; docstring
    documents the child table.
  - `tools/test_mos_xmd.py`: +3 headless tests (child-table chain walk incl.
    `child_start` offsets + `derived_parents`, overrun detection, parent-field
    vs child-table mismatch); the skin helper now emits a matching
    `NODEINDICES` child table.
  - `docs/adr/0418-xmd-skeleton-nodes-nodeindices.md`: `+0x0E` corrected to
    `child_start`; new "Update — 2026-10-16 (wake 8)" section with the
    `FUN_1001e040` evidence.
- Evidence: `mos_xmd.py --inventory <game>\Content\Models --check` → **631/631
  `ok`, 0 failures**. Aggregate via the parser API → **249** skeletons /
  **12593** nodes / **12424 used** + **169 padding** / **12175** indices;
  **0** tiling failures, **0** `child_start` overruns, **0** child_count/indices
  sum mismatches, **0** table-vs-parent mismatches; `max_rotation_slot 73`,
  `max_movement_slot 20`; deepest chain 12. `test_mos_xmd.py` PASS;
  `verify_reported_fixes.py`, `scenario_tests.py --check`,
  `validate_addresses.py`, `verify_hook_init.py` all pass. No C++ changed, so
  `build-all.ps1`/CTest were not rerun (as in Wakes 3–7).
- Blocker: none. Next: extract the Riddick hand/forearm surfaces
  (`RIDDICK_ARMSCAR_D/S/N` in `Riddick_B05.xmd`, `Pa2_Hand_01`) with geometry +
  UVs + skeleton + skin and add a native mesh path behind a flag, keeping
  `VRHandRenderer` as fallback (GH #46). Node names remain unshipped — identify
  the hand bones from geometry/skinning.
- Branch: committed on `main` (`7b33e0e`), pushed to `origin/main`.

## Wake 7 — 2026-10-16 00:03 (scheduled loop)
- **Unit:** GH #55 → #46 — decode the `.xmd` `SKELETON` `NODES`/`NODEINDICES`
  payloads from the shipped `MXR.dll` loader (the skeleton step after
  geometry/UV/skinning of Wakes 3–6).
- Observation (real evidence, read-only; Ghidra 12.1.4 via `rea`): the
  `SKELETON` string is at MXR.dll `.rdata` `0x10149df8`; `rea xrefs` → the model
  writer `FUN_10081f30` and the model reader `FUN_10083060`
  (`CTriangleMeshCore::Read`). The reader calls the skeleton reader
  **`FUN_100219d0`**, which `GetNext("NODES")` then reads one record per node via
  **`FUN_1001fc30`**, `GetNext("NODEINDICES")` (each element a `u16`), optional
  `ATTACHPOINTS`, then finalizes with `FUN_1001ff60`. `FUN_1001fc30` (reader) and
  `FUN_1001eec0` (writer) agree byte for byte: a **34-byte** node record
  (`u16 version==0x100`; `f32 t[3]`; `u16 reserved`; `u16 index`; `u16
  child_count`; `u16 parent`; `f32 scale[2]`; `u16 rotation_slot`; `u16
  movement_slot`). `FUN_1001ff60` caps rotation slots at `0x80` and movement
  slots at `0x20`.
- Recovered: **used** nodes have `rotation_slot == index`; a record with
  `rotation_slot == 0xFFFF` (also `movement_slot == 0xFFFF`, zero translation,
  `parent == 0`) is a **padding** node excluded from `NODEINDICES`.
  `NODEINDICES` = `u16[used_nodes − 1]`, every used non-root node exactly once
  (the bone/traversal order). Node **names are not stored** in `.xmd`.
- Change:
  - `tools/mos_xmd.py`: added `SkeletonNode`, `Skeleton` (`used_nodes`,
    `children_of`, `is_tree`, `child_count_matches`, `max_rotation_slot`,
    `max_movement_slot`), `decode_skeleton`; `Xmd.skeleton()`;
    `geometry_summary()` reports skeleton node/used/padding/index counts + tree
    flag; `validate()` cross-checks the skeleton (version, 34-byte tiling,
    reserved/scale, used `rotation_slot == index`, zeroed padding, `parent` in
    range, acyclic tree, `child_count == used children`, `NODEINDICES == used
    non-root set`); `--skeleton` CLI; docstring documents the layout.
  - `tools/test_mos_xmd.py`: +7 headless synthetic-container skeleton tests
    (round-trip, bad child_count, bad NODEINDICES, size mismatch, non-zeroed
    padding, bad version, absent skeleton); fixed the skin helper to emit a
    valid flat skeleton.
  - `docs/adr/0418-xmd-skeleton-nodes-nodeindices.md` (new).
- Evidence: `mos_xmd.py --inventory <game>\Content\Models --check` → **631/631
  `ok`, 0 failures** (with skeleton cross-checks). Across shipped models: **268**
  `NODES` chunks, **249** non-empty skeletons, **12593** records (**12424 used** /
  **169 padding**), **12175** `NODEINDICES`; **0** violations for every invariant
  above, **249/249** acyclic trees, max rotation slot **73** (< `0x80`), max
  movement slot **20** (< `0x20`). Samples: `Riddick_B05.xmd` 70 nodes / 69
  indices / coherent spine+arm chain (0→1→8→9→10→11→{15,20,24,…});
  `dweller.xmd` 69 records (50 used / 19 padding) / 49 indices.
  `test_mos_xmd.py` PASS; `validate_addresses.py`, `verify_reported_fixes.py`,
  `scenario_tests.py --check`, `verify_hook_init.py` all pass. No C++ changed, so
  `build-all.ps1`/CTest were not rerun (as in Wakes 3–6).
- Blocker: none. Next: extract the Riddick hand/forearm surfaces
  (`RIDDICK_ARMSCAR_D/S/N` in `Riddick_B05.xmd`) with geometry + UVs + skeleton +
  skin and add a native mesh path behind a flag, keeping `VRHandRenderer` as
  fallback (GH #46).
- Branch: committed directly on `main` (sole-weight/single-writer mode); pushed
  to `origin/main` (normal push). No game file modified.

## Wake 6 — 2026-10-15 23:08 (scheduled loop)
- **Unit:** GH #55 → #46 — decode the `.xmd` `BONEDEFORM` skinning payloads from
  the shipped `MXR.dll` loader.
- Observation (real evidence, read-only; Ghidra 12.1.4 via `rea`):
  `CTriangleMeshCore::Read` (`FUN_10080f50`) selects the **embedded**
  `BONEDEFORM` form when the model version is > `0x100` (shipped models report
  `0x102`, read from the first `MISCELLANEOUS` u32) and calls **`FUN_1007d260`**.
  That reader: `u32 n` + `n * 4`-byte vertex records (`{u8 flags; u8
  influence_count; u16 influence_start}`), then `u32 m` + the influence pool read
  by `FUN_1007c150(stream, 0x200, dst, m)` (version-0x200 branch: **2 bytes per
  influence**, `u8 bone` + `u8 weight` scaled by `0.003921569 == 1/255`), then a
  trailing matrix-map block. `influence_start` is the running prefix sum of
  `influence_count`; the loader repairs the last weight to `1 - sum(previous)`.
- Recovered (embedded `BONEDEFORM`, model version `0x102`): payload =
  `u32 n; n*{u8 flags; u8 count; u16 start}; u32 m; m*{u8 bone; u8 wq};
  u32 matrix_map_id (s32, 0xFFFFFFFF = none); u32 b; b*u8`, size
  `4 + 4n + 4 + 2m + 8 + b`. No shipped model uses the separate
  `BONEVERTEXINFO`/`BONEINFLUENCE`/`BONEMATRIXMAP` chunks.
- Change:
  - `tools/mos_xmd.py`: added `BoneVertex`, `BoneDeform` (`influences_of`,
    `max_bone`, `max_influences`), `decode_bonedeform`; `Xmd.bone_deform()`,
    `Xmd._skin_owner_map()` (pairs each `VERTEXFRAMES` with its sibling
    `BONEDEFORM`); `cluster_meshes()` records `skin_chunk`/`skin`/
    `skin_vertices`/`skin_matches`/`max_bone`; `validate()` cross-checks skin/
    vertex counts, prefix-sum starts (in `decode_bonedeform`), exact tiling, and
    bone index < `NODES.count`; `geometry_stats()`/`geometry_summary()` report
    `skinned_clusters`/`skinned_vertices`/`decoded_influences`/
    `max_influences_per_vertex`; `--mesh`/`--cluster N` print skin.
  - `tools/test_mos_xmd.py`: +6 headless synthetic-container skin tests
    (round-trip + weight scale + `influences_of`, matrix-map decode, prefix-sum
    mismatch, skin/vertex count mismatch, out-of-range bone, bad matrix-map
    tiling, no-`BONEDEFORM` case).
  - `docs/adr/0417-xmd-bonedeform-skinning.md` (new); ADR-0415/0416 follow-ups
    marked done.
- Evidence: `mos_xmd.py --inventory <game>\Content\Models --check` → **631/631
  `ok`, 0 failures** (with skinning cross-checks). Aggregate decode → **4662
  clusters**: **3103 skinned** (1559 rigid) / **1,315,626 influences** / **max 3
  influences per vertex**; every skinned cluster tiles exactly (`4+4n+4+2m+8+b`),
  **0** bad clusters, **0** out-of-range bone indices, **0** missing matrix-map
  tails. Per-vertex weights sum to exactly 1.0 on `Riddick_H01`/`B03`/`LGuard`/
  `H_Civ` (**0** violations). Sample `Riddick_H01` cluster 0: 985 verts / 1564
  influences / `max_bone=17` (< 70 skeleton nodes) / `v0 = {3:.008, 1:.102,
  0:.890}`. `test_mos_xmd.py` PASS; `validate_addresses.py`,
  `verify_reported_fixes.py`, `scenario_tests.py --check`, `verify_hook_init.py`
  all pass. No C++ changed, so `build-all.ps1`/CTest were not rerun (as in
  Wakes 3–5).
- Blocker: none. Next: decode the `NODES` skeleton (names + hierarchy) +
  `NODEINDICES` so bone indices resolve to named joints; then extract the
  Riddick hand/forearm surfaces (`RIDDICK_ARMSCAR_*`, `Pa2_Hand_01`) with
  geometry + UVs + skin and add a native mesh path behind a flag, keeping
  `VRHandRenderer` as fallback.
- Branch: committed on `docs/rea-mos-loader-layout` (single-writer mode — no new
  branch; not pushed; `main` untouched).

## Wake 5 — 2026-10-15 22:48 (scheduled loop)
- **Unit:** GH #55 → #46 — decode the `.xmd` `TVERTEXFRAMES` (UV) payloads from
  the shipped `MXR.dll` loader.
- Observation (real evidence, read-only; Ghidra 12.1.4 via `rea`): the
  `TVERTEXFRAMES` per-frame reader is **`FUN_100807d0`** in `MXR.dll`
  (`CTriangleMeshCore::Read` calls it once per cluster right after reading
  `VERTEXFRAMES`; `FUN_1007fce0` sizes the UV list, `FUN_1007fba0` the position
  list). It switches on a version argument: `0x201` = deferred/seeking form
  (`RelSeek(count*4 + 0x10)`), `0` = raw `u32[n][2]`, else
  `throw "Unsupported version %.4x"`. The branch every shipped asset selects is
  **`0x200`**.
- Recovered (VF version 0x200 UV frame): `u32 n; f32 min_u; f32 min_v; f32
  max_u; f32 max_v; u16 uv[n][2]`; dequant `min + q*(max-min)/65535` (same
  `1.5259022e-05` constant as positions); frame size exactly **`20 + 4n`**. A
  cluster with no UVs stores an **empty** (`data_size == 0`) `TVERTEXFRAMES`
  node.
- Change:
  - `tools/mos_xmd.py`: added `TVertexFrame`, `decode_tvertex_frame`,
    `Xmd.tvertex_frames()`; `cluster_meshes()` now pairs each cluster's
    `VERTEXFRAMES`/`TVERTEXFRAMES`/`TRIANGLES` and records `uv_count`/
    `uv_matches`; `validate()` cross-checks exact UV tiling + UV/vertex count
    agreement; `geometry_stats()`/`geometry_summary()` report `uv_clusters`/
    `decoded_uvs`; docstring documents the layout; `--mesh`/`--cluster N` print
    UVs.
  - `tools/test_mos_xmd.py`: +4 headless synthetic-container UV tests
    (round-trip/dequant, count mismatch, bad tiling, empty chunk).
  - `docs/adr/0416-xmd-uv-tvertexframes.md` (new); ADR-0415 follow-up marked
    done.
- Evidence: `mos_xmd.py --inventory <game>\Content\Models --check` → **631/631
  `ok`, 0 failures**. Aggregate decode → **4662 clusters**: **2099 carry UVs**
  (each tiling exactly `20+4n`, `n` == paired vertex count) and **2563 store an
  empty node** (untextured); **423,559** decoded UV pairs; **0** bad clusters,
  **0** UV/vertex mismatches. Sample `Riddick_H01.xmd` cluster 0:
  985 verts / 985 UVs / `uv_match=True`. `test_mos_xmd.py` PASS;
  `validate_addresses.py`, `verify_reported_fixes.py`, `scenario_tests.py
  --check`, `verify_hook_init.py` all pass. No C++ changed, so
  `build-all.ps1`/CTest were not rerun (as in Wakes 3–4).
- Blocker: none. Next: decode `BONEDEFORM`/`BONEINFLUENCE`/`BONEMATRIXMAP`
  (skinning; `FUN_1007d260` = BONEDEFORM reader — per-vertex
  `(u16 bone_index, u8 influence_count, ...)` + per-influence `u16 bone,
  f32 weight`), then extract the Riddick hand/forearm surfaces
  (`RIDDICK_ARMSCAR_*`, `Pa2_Hand_01`) with geometry + UVs + skin and add a
  native mesh path behind a flag, keeping `VRHandRenderer` as fallback.
- Branch: committed on `docs/rea-mos-loader-layout` (single-writer mode — no new
  branch; not pushed; `main` untouched).

## Wake 4 — 2026-10-15 22:40 (scheduled loop)
- **Unit:** GH #55 → #46 — decode the `.xmd` `VERTEXFRAMES`/`TRIANGLES`
  geometry payloads from the shipped `MXR.dll` loader.
- Observation (real evidence, read-only; Ghidra 12.1.4 via `rea`): the geometry
  chunk-name strings + readers live in **`MXR.dll`** (sha256 `06424f55…`, PDB
  `Dll_XR.pdb`), not `MCCDyn.dll`. `FUN_10080f50` = `CTriangleMeshCore::Read`;
  `FUN_1007fed0` = `CVertexFrame::Read` (switches on version `0x200`..`0x204`,
  else throws `"Unsupported vertex-frame version %.4x"`); positions are decoded
  by `FUN_1007c6c0`, normals by `FUN_1007ca20`; `FUN_10006620` = read `u32`,
  `FUN_10019390` = read 3 floats.
- Recovered (all shipped models use VF **version `0x204`**): frame =
  `u32 version; u32; f32 min[3]; f32 max[3]; f32 aux[3]; u32; u32 n;
  u16 pos[n][3]; s8 nrm[n][3]`; positions dequantized
  `min + q*(max-min)/65535` (`1.5259022e-05 == 1/65535`), normals `c/127`
  (`0.007874016 == 1/127`) re-normalized; frame size **`52 + 9n`**; when the
  model version is > `0x100` the chunk starts with one `u32` frame-count word
  (read + discarded). `TRIANGLES` = `u32 count` + `count` *u16* triplets.
- Change:
  - `tools/mos_xmd.py`: added `VertexFrame`, `decode_vertex_frame`,
    `decode_triangles`; `Xmd.vertex_frames()/triangle_indices()/
    cluster_meshes()/geometry_stats()`; `validate()` now cross-checks every
    `VERTEXFRAMES`/`TRIANGLES` cluster pair (index range + payload tiling);
    `--mesh`/`--cluster N` CLI; docstring documents the layout.
  - `tools/test_mos_xmd.py`: +3 headless synthetic-container geometry tests
    (round-trip, out-of-range index, triangle-size mismatch).
  - `docs/adr/0415-xmd-vertex-frame-geometry.md` (new); ADR-0414 follow-up
    marked done.
- Evidence: `mos_xmd.py --inventory <game>\Content\Models --check` → **631/631
  `ok`** (geometry cross-check on); aggregate decode → **4662/4662 cluster
  pairs** with every triangle index in range *and* `max(index) == n-1` exactly
  (0 unsupported, 0 corrupt); totals **1,242,285 vertices / 1,860,184
  triangles**; the `52+9n` frame size tiles every chunk with no remainder;
  `test_mos_xmd.py` PASS; `validate_addresses.py`, `verify_reported_fixes.py`,
  `scenario_tests.py --check`, `verify_hook_init.py` all pass. No C++ changed, so
  `build-all.ps1`/CTest were not rerun (as in Wake 3).
- Blocker: none. Next: decode `TVERTEXFRAMES` (UV; node flags `0x201`) and
  `BONEDEFORM`/`BONEINFLUENCE`/`BONEMATRIXMAP` (skinning), then extract the
  Riddick hand/forearm surfaces (`RIDDICK_ARMSCAR_*`, `Pa2_Hand_01`) and add a
  native mesh path behind a flag, keeping `VRHandRenderer` as fallback.
- Branch: committed on `docs/rea-mos-loader-layout` (single-writer mode — no new
  branch; not pushed; `main` untouched).
  - Note: `rea --snapshot <name>` writes `<name>` into the CWD — do **not**
    commit it (the `mxr` snapshot was removed from the tree).

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

## Wake 2 — 2026-10-15 (interactive, deep RE)
- Ghidra analysis of the model pipeline via REA; committed
  `docs/rea/mos-xmd-format.md` + `docs/rea/reference/*.txt` (pseudocode).
- Container = generic **`CDataFile`** in `MCCDyn.dll`
  (`Create` xref `0x10082a74`, `ReadTree` xref `0x10082f15`); confirmed the
  `tools/mos_xmd.py` layout (0x30 header + 0x30 node records).
- Geometry reader = **`CTriangleMeshCore::Read`** in `MXR.dll`
  (`VERTEXFRAMES`/`TVERTEXFRAMES`/`BONEDEFORM` via `CDataFile::GetNext`).
- Next: finish `VERTEXFRAMES`/`BONEDEFORM` byte layouts, recover bone names,
  export OBJ, add skinned draw path. See GH #55.

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
