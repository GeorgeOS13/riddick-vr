# ADR 0419: `.xmd` mesh/skin export pipeline (`tools/xmd_export.py`)

## Status
**Accepted** - the decoded `.xmd` clusters, skeleton and skin weights can now be
exported to portable artifacts (Wavefront OBJ + JSON) and round-tripped on every
shipped model. Read-only analysis; no game file was modified and no code is
deployed.

This is the **extraction/export** step of GH #55 (advancing GH #46), following
the container layout of ADR-0414, the position/triangle geometry of ADR-0415, the
UVs of ADR-0416, the skinning of ADR-0417 and the skeleton of ADR-0418.

## Context
ADR-0414..0418 recovered the whole `.xmd` structure, so a mesh can be decoded
with positions, normals, UVs, triangle indices, per-vertex bone influences and a
skeleton. The remaining gap to a **native first-person hand/arm path** (GH #46)
was turning that decoded data into something a renderer can consume without
re-deriving the binary format. `docs/rea/mos-xmd-format.md` "Next steps" #3 and
`tools/autoloop/STATE.md` (wake 8) both call for exactly this: emit the Riddick
arm/hand surfaces (`RIDDICK_ARMSCAR_*` in `Riddick_B05.xmd`, `Pa2_Hand_01`).

## Change
- `tools/xmd_export.py` (new): read-only exporter built on `tools/mos_xmd.py`.
  Decodes any model's clusters and writes
  - `<prefix>_c<NN>.obj` - one Wavefront OBJ per cluster (`v`/`vn`/`vt`/`f`);
  - `<prefix>.obj` - optional (`--combined`) single OBJ with rebased indices;
  - `<prefix>.skin.json` - the skeleton hierarchy + per-vertex bone influences;
  - `<prefix>.export.json` - a compact, committable manifest (counts, bounds,
    bones used per cluster).
  `--list` prints the per-cluster summary without writing; `--cluster N`
  restricts the export.
- `tools/test_xmd_export.py` (new): 6 headless synthetic-container tests
  (OBJ round-trip incl. UV/no-UV faces and combined-index rebasing, skin JSON
  weights, cluster selection + out-of-range, manifest bone/surface stats,
  skeleton child table, CLI errors).
- `.github/workflows/vr-worker.yml`: run the new test alongside
  `test_mos_xmd.py`.
- `docs/rea/reference/riddick_b05_export.json`,
  `docs/rea/reference/pa2_hand_01_export.json`: generated manifests (evidence).

## Evidence (2026-10-16, read-only)
- **All shipped models export cleanly.** `Content/Models/**` -> **631** files,
  **631 ok / 0 fail**. Totals decoded and emitted: **4662 clusters**,
  **1,242,285 vertices**, **1,860,184 triangles**, **423,559 UVs**, **3103**
  skinned clusters. For every cluster the emitted OBJ was re-parsed: the `v`
  count equals the decoded vertex count, the `f` count equals `triangles/3`, and
  every face index is in range. (Matches the geometry totals of ADR-0415/0417.)
- `tools/test_xmd_export.py` PASS; `tools/test_mos_xmd.py` PASS;
  `validate_addresses.py`, `verify_reported_fixes.py`,
  `scenario_tests.py --check`, `verify_hook_init.py` all pass. No C++ changed,
  so `build-all.ps1`/CTest were not rerun (as in wakes 3-8).
- **`Riddick_B05.xmd`** (biped, 70-node skeleton, 7 clusters): cluster bounds
  and bones used (see `docs/rea/reference/riddick_b05_export.json`):

  | cluster | verts | tris | UVs | bounds (x, y, z) | bones used |
  |---|---|---|---|---|---|
  | 0 | 372 | 530 | 372 | x -4.0..7.7, y -1.87..1.87, z 0.0..25.0 | 0-9 |
  | 1 | 526 | 807 | 526 | x -4.7..3.6, y -25.4..-5.5, z 45.1..52.5 | 0-23 |
  | 2 | 344 | 547 | 344 | x -3.5..1.3, y 17.6..25.4, z 44.2..50.5 | 0-19 |
  | 3 | 372 | 530 | 0 | identical to 0 | 0-9 |
  | 4 | 526 | 807 | 0 | identical to 1 | 0-23 |
  | 5 | 344 | 547 | 0 | identical to 2 | 0-19 |
  | 6 | 956 | 1884 | 0 | x -4.4..7.7, y -6.9..6.9, z 0.0..36.3 | 49 |

  - Clusters **3,4,5 duplicate 0,1,2** (same bounds, vertex/triangle counts and
    bone sets); the duplicates store an **empty** `TVERTEXFRAMES` node. So each
    of the 3 surfaces appears twice: a textured copy and an untextured copy.
  - Cluster 0/3 are the **legs** (bones 0-9, z 0-25); clusters 1/4 and 2/5 are
    the **two arms** (z ~44-52, `|y|` up to 25.4, bones 10-23); cluster 6 is the
    untextured `SHADOWDATA` body.
- **`Pa2_Hand_01.xmd`**: 2 clusters, **678** verts / **1256** tris, cluster 0
  textured (138/138), cluster 1 untextured; **no** `BONEDEFORM`/`NODES`
  (unskinned static hand).

## Update - `NODES` translations are model-space, not local offsets
While validating the export the `NODES` `translation[3]` field was re-checked
against the geometry. ADR-0418 called it a "bind-pose local translation"; the
data shows it is the node's **bind-pose model-space position** (the same
coordinate space as `VERTEXFRAMES`).

- **Decisive single-model evidence** (`Riddick_B05.xmd`): node 1 (pelvis,
  `parent 0`) has `t = (0.713, 0.0, 32.617)` and node 2 (left hip, `parent 1`)
  has `t = (0.713, -2.495, 32.617)`. Read as a *local child offset*, node 2 would
  sit at `z = 65.2` on a figure whose head is at `z ~= 54.8` - impossible; and a
  local offset would not share its parent's own `z`. Read as a model-space
  position, node 1 is the pelvis centre and node 2 the left hip at the same
  height, matching the leg mesh bounds (cluster 0, `z 0..25`). Likewise node 15
  (`parent 11`, `z 54.776`) is at `z 56.065`.
- **Aggregate check** over all **248** skeletons: summing translations along the
  parent chain (the "local" reading) pushes nodes outside the union mesh bbox by
  more than 25% of its span in **196/248** models (worst **2160x** span); the
  direct reading does so in only **96/248** (worst **351x**, and that residue is
  partial-mesh models - e.g. head/gib models that ship the full skeleton but only
  a small mesh subset, so the test is conservative against the direct reading).
- Consequence: the skeleton exposes bind-pose **positions** in model space but no
  rotations (those live in `KEYS`/animation), so a consumer can build bind
  translations directly and must still supply per-bone rotations. `mos_xmd.py`
  docstrings corrected accordingly.

## Unknowns / follow-ups
- **Bone-index space.** The `BONEDEFORM` influence byte is `< 0x80` and every
  used node has `rotation_slot == index` (ADR-0418), so the byte reading as the
  **skeleton node index / animation rotation slot** is the natural single space;
  the alternative (1-based position in `NODEINDICES`) is not fully excluded by a
  spatial-centroid heuristic and should be confirmed against the skinning
  consumer in the engine.
- The exact `SURFACES`-to-`CLUSTER` mapping (which of clusters 0-5 carries
  `RIDDICK_ARMSCAR_*`) is not yet pinned. `SURFACES` reports `count == 3` but
  holds 12 name strings (material name + up to 3 texture-map refs each).
- `PRIMITIVES`, `SOLIDS` and `ATTACHPOINTS` remain undecoded.
- **Next:** bake the exported arm/hand mesh + skeleton into a runtime-loadable
  form and add the native skinned draw path in `VRHandRenderer` **behind a config
  flag**, keeping the procedural overlay as the fallback (GH #46 / #55).
