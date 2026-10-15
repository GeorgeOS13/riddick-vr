# ADR 0418: `.xmd` skeleton — `NODES` / `NODEINDICES` layout recovered from MXR.dll

## Status
**Accepted** — the `SKELETON` `NODES` / `NODEINDICES` payload layouts are decoded
from the shipped loader and re-validated across every shipped skeleton (249
models, 12593 node records). Read-only analysis; no game file was modified and no
code is deployed.

This is the skeleton step of GH #55 (advancing GH #46), following the container
layout of ADR-0414 (`.xmd` container) and the geometry/UV/skinning of
ADR-0415/0416/0417.

## Context
ADR-0415/0416/0417 recovered the per-cluster geometry (`VERTEXFRAMES`),
UVs (`TVERTEXFRAMES`) and skinning (`BONEDEFORM`), so a decoded mesh can be
skin-weighted. The missing piece was the **skeleton**: which node each bone index
refers to and how the nodes are parented, so a skinned mesh can be posed and the
Riddick hand/forearm surfaces can be attached to tracked controllers (GH #46).
`BONEDEFORM` bone indices address the `NODES` array (validated in ADR-0417), but
the node records and the `NODEINDICES` order were still opaque.

## Evidence (2026-10-15/16, read-only; Ghidra 12.1.4 via `rea`)
Target: `System/Win32_x86/MXR.dll` (sha256
`06424f55f5a0f85eb09b5fbd69491c7f6a88de808c26c513e8dac180f3c2052d`,
PDB `Dll_XR.pdb`; image base `0x10000000`).

- The `SKELETON` literal lives in `.rdata` at `0x10149df8`. `rea xrefs` resolves
  exactly two referencing procedures: `FUN_10081f30` (the model **writer**) and
  `FUN_10083060` (the model **reader**, `CTriangleMeshCore::Read` for the
  top-level `EXTENDEDMODEL`).
- The model writer `FUN_10081f30` emits, in order: `VERSION` (`0x302`),
  `SURFACES`, `BOUND`, `CLUSTERS` (each cluster via `FUN_1007d7b0`), `SOLIDS`,
  **`SKELETON`** → `FUN_1001f7a0(skeleton, stream)`, `KEYS`, `LODBIAS`,
  `SHADOWDATA`. The model reader `FUN_10083060` reads `SKELETON` then a
  sub-directory and calls **`FUN_100219d0(skeleton, stream)`**.
- `FUN_100219d0` (skeleton reader): `CDataFile::GetNext("NODES")`, size the node
  list to the entry count and read one record per node via `FUN_1001fc30`;
  `CDataFile::GetNext("NODEINDICES")`, then read each element as a `u16`
  (`FUN_10017650`); optional `ATTACHPOINTS` (`FUN_1001fdc0`, stride `0x10`);
  finish with `FUN_1001ff60`.
- `FUN_1001fc30` (node reader) = `FUN_1001eec0` (node writer), byte for byte::

      u16  version            (must be 0x100, else "Unsupported node version")
      u32  -> +0x00           f32 x   )
      u32  -> +0x04           f32 y   ) bind-pose local translation
      u32  -> +0x08           f32 z   )
      u16  -> +0x0C           reserved (0 in every shipped asset)
      u16  -> +0x0E           child_start (first child's slot in NODEINDICES)
      u16  -> +0x10           child_count
      u16  -> +0x12           parent
      u32  -> +0x14           f32     ) scale (1.0, 1.0 in every shipped asset)
      u32  -> +0x18           f32     )
      u16  -> +0x1C           rotation_slot
      u16  -> +0x1E           movement_slot

  (`FUN_10006860` = write/read `u32`; `FUN_1001e2d0` / `FUN_10017650` = `u16`.)
- `FUN_1001ff60` (finalize) scans every node's `+0x1C` and `+0x1E`:
  `param[9] = max(+0x1C) + 1` ("Too many rotation-slots used." if `> 0x80`) and
  `param[10] = max(+0x1E) + 1` ("Too many movement-slots used." if `> 0x20`).
  So the two trailing `u16` are the animation **rotation** and **movement**
  channel slots.

## Recovered layout
```
NODES        : N * 34-byte records, version 0x100 (fields above).

               A record with rotation_slot == 0xFFFF is a PADDING node: it also
               carries movement_slot == 0xFFFF, translation (0,0,0) and
               parent == 0, and is excluded from NODEINDICES. Used nodes carry
               rotation_slot == their own index, and child_count counts only
               their used children.

NODEINDICES  : u16[used_nodes - 1], the flattened child table; node i's children
               are NODEINDICES[child_start_i : child_start_i + child_count_i].
               The used nodes' ranges tile the table exactly, so it lists every
               used non-root node exactly once — the skeleton's bone/traversal
               order. See the update below for how this was pinned down.

(ATTACHPOINTS is present when non-empty; stride 0x10, not decoded here.)
```

## Verification
- `python tools/mos_xmd.py --inventory <game>\Content\Models --check`:
  **631/631 files `ok`, 0 failures** (with the skeleton cross-checks enabled).
- Across all shipped models: **268** files carry a `NODES` chunk, of which
  **249** have a non-empty skeleton — **12593** node records, **12424 used**,
  **169 padding**; **12175** `NODEINDICES` entries (= used − one root per model).
- Every rule below holds with **0 violations**, an independent arithmetic/structural
  check that the field order and widths are right:
  - node `version == 0x100`; `NODES` size `== 34 * N` exactly;
  - reserved field `== 0`; `scale == (1.0, 1.0)`;
  - used node `rotation_slot == index`; padding nodes zeroed with
    `movement_slot == 0xFFFF`;
  - `child_count == number of used children`;
  - `parent ∈ [0, N)` and the used nodes form an acyclic tree rooted at node 0
    (**249/249**);
  - `NODEINDICES` is exactly the used non-root node set (no duplicates, no
    out-of-range values).
  - `max rotation slot == 73` (`< 0x80`) and `max movement slot == 20` (`< 0x20`),
    consistent with `FUN_1001ff60`.
- Samples: `Riddick_B05.xmd` → 70 nodes / 69 indices / tree, a coherent spine+arm
  chain (0 → 1 → 8 → 9 → 10 → 11 → {15,20,24,…}); `dweller.xmd` → 69 records
  (50 used, 19 padding) / 49 indices.
- `tools/test_mos_xmd.py` extended with headless synthetic-container skeleton
  tests (round-trip, bad child_count, bad NODEINDICES, size mismatch,
  non-zeroed padding, bad version, absent skeleton).

## Consequences
- Bone indices from `BONEDEFORM` (ADR-0417) now resolve to a named-position
  skeleton node with parent/child relationships and a bind-pose translation, so
  a decoded, skin-weighted mesh can be posed toward the tracked-hand path of
  GH #46.
- **Node names are not stored** in `.xmd`; the skeleton exposes numeric ids,
  hierarchy, bind-pose translations and animation slots only. Identifying the
  hand/forearm bones must come from the geometry/surface side
  (`RIDDICK_ARMSCAR_*`) or the animation groups (`.xsa`/`.xag`), not from the
  node records.

## Unknowns / follow-ups
- The `rotation`/`movement` slot semantics (animation-channel addressing) are
  decoded structurally but not yet interpreted.
- `ATTACHPOINTS` (stride `0x10`) is not decoded.
- Next: extract the Riddick hand/forearm surfaces (`RIDDICK_ARMSCAR_D/S/N` in
  `Riddick_B05.xmd`) with geometry + UVs + skeleton + skin, and add a native mesh
  path behind a config flag, keeping `VRHandRenderer` as the fallback
  (GH #46 / #55).

## Update — 2026-10-16 (wake 8): `+0x0E` is `child_start`; `NODEINDICES` is the child table
The first pass left the `+0x0E` node field uninterpreted ("index") and treated
`NODEINDICES` only as the *set* of used non-root nodes. A second decompilation
pass pins both down exactly.

- **Evidence** (read-only; Ghidra 12.1.4 via `rea`). The load-time hierarchy
  builder **`FUN_1001e040`** (called by `FUN_1001f2e0`, itself called at the end
  of the skeleton reader `FUN_100219d0`) recurses over the tree. For node
  `param_1` it computes the record address `nodes_base + param_1 * 0x20` and,
  with `uVar4 = *(u8*)(node + 0x10)` (**child_count**) and
  `iVar5 = *(i16*)(node + 0x0e)` (**child_start**), reads each child as
  `*(u16*)(NODEINDICES_base + (iVar5 + child) * 2)` and recurses on it. It never
  reads the `parent` field (`+0x12`). So `+0x0E` is the slot of the node's first
  child in `NODEINDICES`, and `NODEINDICES` is the **flattened child table**.
- **Re-validated layout** (edges now derived from the table, the `parent` field
  cross-checked as its inverse):
  - every used node's `child_start + child_count <= len(NODEINDICES)`;
  - the used nodes' `[child_start, child_start + child_count)` ranges **tile**
    `NODEINDICES` exactly (each entry owned once);
  - `sum(child_count) == len(NODEINDICES) == used_nodes - 1`;
  - `children_of(i) == NODEINDICES[child_start_i : child_start_i + child_count_i]`;
  - the table-derived parent of every used non-root node equals its `parent`
    field (the field is the inverse of the table, not an independent tree).
- **Verification on shipped content** (`tools/mos_xmd.py` API, all models):
  **249** skeletons / **12593** nodes / **12424 used** (+**169 padding**) /
  **12175** indices; **0** tiling failures, **0** `child_start` overruns, **0**
  `child_count`/indices-sum mismatches, **0** table-vs-parent-field mismatches;
  `max rotation slot 73`, `max movement slot 20`; deepest chain 12.
  `--inventory ... --check` remains **631/631 `ok`, 0 failures**.
- `tools/mos_xmd.py`: `SkeletonNode.child_start` replaces `unknown_e`;
  `Skeleton.children_of()` now reads the table (mirroring `FUN_1001e040`), and
  `child_ranges_tile()` / `derived_parents()` / `child_count_matches()` add the
  new cross-checks; `--check` and `geometry_summary()` report them.
  `tools/test_mos_xmd.py`: +3 headless tests (child-table chain walk, overrun,
  parent-field-vs-table mismatch).
- Consequence: the skeleton hierarchy is now recovered exactly as the engine
  walks it (child-list order, not just parent pointers), which is what a native
  skinned-hand path needs to pose the arm chain to tracked controllers.

## Update - 2026-10-16 (wake 9): `translation[3]` is model-space, not local
The field labelled "bind-pose local translation" above is actually the node's
**bind-pose model-space position** (same space as `VERTEXFRAMES`). Evidence: in
`Riddick_B05.xmd`, node 2 (`parent 1`) shares node 1's `z == 32.617` at the hips;
read as a local child offset it would land at `z = 65.2` on a ~52-unit figure.
The full evidence and the aggregate check over 248 skeletons are in
`docs/adr/0419-xmd-mesh-skin-export.md` ("Update - NODES translations are
model-space"). `tools/mos_xmd.py` docstrings corrected.
