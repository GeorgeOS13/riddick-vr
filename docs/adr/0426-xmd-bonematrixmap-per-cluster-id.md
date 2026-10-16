# ADR 0426: the `.xmd` trailing block is the engine's `BONEMATRIXMAP`; `matrix_map_id` is a per-cluster id, not a shared palette key

## Status
**Accepted** — the mesh field at `+0x24` (with its signed id at `+0x28`) is the
engine's **bone matrix map** (the legacy `BONEMATRIXMAP` chunk), and it is **not**
the per-cluster reference/bind-inverse transform GH #59 is looking for. Across
all 631 shipped models the id does **not** group clusters by palette: the three
duplicate `Riddick_B05` cluster pairs carry **identical palettes with different
ids** (`-2/0`, `-2/1`, `-2/2`), and 462/488 within-model id groups have differing
palettes. Measured read-only; no game file is modified and no code is deployed.

This closes the "`matrix_map_id` registry relationship" open item from ADR-0423
and rules the field out as the GH #59 per-cluster transform.

## Context
ADR-0423 pinned the `BONEDEFORM` trailing block as `s32 matrix_map_id; u32 b;
b × u8` and treated the bytes as a `matrix_bones` **palette** (an influence
`bone` byte is an index into it when non-empty). ADR-0425 ruled out the
`CLUSTER`/`MISCELLANEOUS` header as the per-cluster transform, leaving "the
`matrix_map_id` registry (ADR-0423) or a runtime per-cluster matrix in the
model/skin class" as the remaining candidates. This ADR identifies the field and
tests the registry hypothesis against the shipped data.

## Finding 1 — the field is the `BONEMATRIXMAP` chunk (legacy name)
`CTriangleMeshCore::Read` (`MXR.dll` `FUN_10080f50`) has two paths:

- **Legacy (model version ≤ `0x100`):** it reads three *named* sibling chunks into
  the mesh object:
  - `GetNext("BONEVERTEXINFO")` → `mesh+0x18` (per-vertex influence records),
  - `GetNext("BONEINFLUENCE")` → `mesh+0x20` (the influence pool, via
    `FUN_1007c150`),
  - `GetNext("BONEMATRIXMAP")` → `mesh+0x24` (`FUN_1007b630` allocates the list,
    then a raw `CCFile::Read` fills it).
  It then sets `flags |= 0x4000` ("has bone deform") when the vertex records are
  non-empty, and `flags |= 0x8000` ("has bone matrix map") when `mesh+0x24` is
  non-empty.
- **Modern (version > `0x101`, shipped `0x102`):** the same three roles are packed
  into one embedded `BONEDEFORM`, read by `FUN_1007d260`, which writes the signed
  id to `mesh+0x28` and the palette to `mesh+0x24` (the *same* fields).

So `mesh+0x24` is the **bone matrix map** container and `mesh+0x28` its id — a
per-cluster registry slot, not a geometry transform. The `MXR.dll` RTTI/strings
list the matching class/consumer surface:
`CXR_Model_TriangleMesh::Cluster_TransformBones_V_N_TgU_TgV`.

## Finding 2 — the loader does not apply the palette
`FUN_1007c150` (influence-pool reader, version `0x200`) reads the pool as
`{u8 bone; u8 weight_q}` pairs and expands each into an 8-byte runtime record
`{u16 bone; pad; f32 weight}` (`weight = q/255`) **verbatim** — no palette
lookup. Palette→node resolution is therefore deferred to the runtime skinning
consumer, not done at load time.

`FUN_10081b10` (`"CreateMatrixIW"`) is that consumer's data prep: for each vertex
it packs up to **4** bone indices, one byte each, into a `u32` at `mesh+0x30`
(and 3 weights per vertex at `mesh+0x34`), throwing
`"Too many bones/vertex %d/4 (%s)"` if any vertex has more than four influences.
The packed bytes are the palette indices, i.e. indices into the bone matrix array.

## Finding 3 — `matrix_map_id` is per-cluster, not a shared key
Corpus (631 models / 4662 clusters / 3103 skinned / **2411 paletted**):

| property | value |
|---|---|
| distinct `matrix_map_id` values | **11** |
| `matrix_map_id` histogram (paletted) | `-6:1  -5:2  -4:6  -3:2  -2:1166  -1:33  0:721  1:292  2:137  3:50  4:1` |
| within-model id groups (≥1 paletted cluster) | **657** |
| of those, with >1 cluster | **488** |
| — palettes **agree** | **26** |
| — palettes **differ** | **462** |
| within-model palette groups with >1 cluster | **1007** (1006 have differing ids) |
| ids whose palette varies across models | **8 / 11** |

Decisive — `Riddick_B05.xmd`, the duplicate clusters ADR-0419/0424 established
(c0↔c3, c1↔c4, c2↔c5 share geometry):

| cluster | part | `matrix_map_id` | palette length |
|---|---|---|---|
| 0 / 3 | legs mesh | **-2 / 0** | 10 / 10 (identical) |
| 1 / 4 | left arm | **-2 / 1** | 24 / 24 (identical) |
| 2 / 5 | right arm | **-2 / 2** | 20 / 20 (identical) |
| 6 | shadow body | -1 | 0 (direct node space) |

Identical palettes with different ids is impossible if the id named a shared
bone-set registry. Conversely id `-2` alone spans the legs, left-arm and right-arm
clusters (three different palettes) in this one model.

**Conclusion:** `matrix_map_id` is a **per-cluster identifier** (an engine
registration slot), not a palette/registry key. The field is not the missing
per-cluster reference transform.

## Change
- `tools/mos_xmd.py`: `Xmd.matrix_map_groups()` (per-id cluster groupings +
  palette agreement; empty-palette clusters excluded); `geometry_stats()`/
  `geometry_summary()` report `matrix_map_ids` (id → paletted-cluster count),
  `matrix_map_shared_id_groups` and `matrix_map_palette_conflict_groups`;
  `report_file()`/`--matrix-map` prints the groups; module docstring documents the
  `BONEMATRIXMAP` identity.
- `tools/test_mos_xmd.py`: **+3** tests (conflicting-palette id group,
  agreeing-palette id group, direct-node clusters excluded) → **50** total.
- `docs/rea/reference/MXR_CreateMatrixIW.txt`,
  `docs/rea/reference/MXR_ReadBoneInfluence.txt`: pseudocode evidence dumps.
- `docs/rea/reference/xmd_matrix_map_registry.json`: the aggregate table above
  plus the `Riddick_B05` cluster ids/palettes.
- `docs/rea/mos-xmd-format.md` `BONEDEFORM` section; ADR-0423/0425 cross-refs.

## Evidence (2026-10-16, read-only)
- `rea decompile MXR.dll --procedure 0x10080f50 / 0x1007d260 / 0x1007c150 /
  0x10081b10` (Ghidra 12.1.4) — the `BONEMATRIXMAP` read, the verbatim pool
  reader, and the 4-bone `CreateMatrixIW` packing.
- `python tools/mos_xmd.py --inventory <game>\Content\Models --check` →
  **631/631 `ok`, 0 FAIL**.
- Aggregate via `tools/mos_xmd.py` / `xmd_matrix_map_registry.json` → the table
  above (2411 paletted clusters; 488 multi-cluster id groups, 462 conflicting).
- `python tools/mos_xmd.py <game>\...\Riddick_B05.xmd --matrix-map` →
  `matrix_map_id=-2 clusters=[0, 1, 2] palettes=3 agree=False` and `0/1/2` each a
  singleton identical to one of those palettes.
- `tools/test_mos_xmd.py` (50) PASS; `test_xmd_export.py`, `test_xmd_bake.py`,
  `test_xmd_pose.py`, `test_xsa_anim.py`, `validate_addresses.py`,
  `verify_hook_init.py`, `verify_reported_fixes.py`, `scenario_tests.py --check`
  all PASS.

## Consequences
- **GH #59 narrows:** neither the `MISCELLANEOUS` header (ADR-0425) nor the
  `BONEMATRIXMAP` palette/id (this ADR) is the per-cluster reference transform.
  The remaining candidate is a **runtime** per-cluster matrix built by the
  model/skin class from the live skeleton (bind-inverse × current bone matrix),
  i.e. supplied by the animation/pose state, not stored in the model file.
- The `BONEMATRIXMAP` palette is genuinely the bone-index space for a skinned
  draw (ADR-0423), now named and sourced; the id is engine bookkeeping and can be
  ignored by an offline consumer.
- `--check` gains the id-histogram/conflict metrics, which will flag any future
  assumption that the id groups bone sets.

## Reproduce
```
rea decompile "<game>\System\Win32_x86\MXR.dll" --procedure 0x10080f50
rea decompile "<game>\System\Win32_x86\MXR.dll" --procedure 0x10081b10
python tools/mos_xmd.py <game>\Content\Models\Characters\Riddick\Riddick_B05.xmd --matrix-map
python tools/test_mos_xmd.py
```

## Files
- `tools/mos_xmd.py`, `tools/test_mos_xmd.py`
- `docs/rea/mos-xmd-format.md`
- `docs/rea/reference/MXR_CreateMatrixIW.txt`,
  `docs/rea/reference/MXR_ReadBoneInfluence.txt`,
  `docs/rea/reference/xmd_matrix_map_registry.json`
- cross-refs: ADR-0417, ADR-0423, ADR-0424, ADR-0425
