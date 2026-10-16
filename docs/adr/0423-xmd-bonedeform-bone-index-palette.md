# ADR 0423: `.xmd` `BONEDEFORM` influence indices are a `matrix_bones` palette

## Status
**Accepted** — the `BONEDEFORM` per-influence `bone` byte is an index into the
trailing `matrix_bones` palette (which yields the skeleton node) whenever that
palette is non-empty, and a direct skeleton node index only when the palette is
empty. Re-validated across all 631 shipped models (4662 clusters, 3103 skinned).
Read-only analysis; no game file was modified and no code is deployed.

This closes the "node index vs `NODEINDICES` position" question left open by
ADR-0417 and ADR-0419, and is the skinning companion of the animation decode
(ADR-0422) on the way to the native hand/arm draw path (GH #46 / GH #55).

## Context
ADR-0417 recovered the embedded `BONEDEFORM` layout and its trailing block
(`s32 matrix_map_id; u32 b; b × u8`) but treated the `matrix_bones` bytes as
opaque and assumed an influence's `bone` byte was already a skeleton node index
(its `--check` only asserted `bone < NODES.count`). ADR-0419 then recorded the
per-cluster "bones used" as those raw bytes (arm clusters reported `0..23`),
which conflicted with the geometry: the arm mesh sits at `z ≈ 45..52` while the
`0..23` skeleton nodes are the pelvis/legs/spine (`z 0..48`). The bone-index
space had to be pinned before weights could drive a real skinned pose.

## Finding — the `matrix_bones` palette remaps influence indices to node indices
1. **Space rule.** When `matrix_bones` (length `b`) is **non-empty** the
   influence `bone` byte is a **palette index**: `skeleton_node =
   matrix_bones[bone]`. When `matrix_bones` is **empty** the byte is the
   **skeleton node index** directly.
2. **`matrix_map_id` is not the discriminator.** It is `-1` for the 692
   map-less clusters *and* also `-1` for **33** clusters that carry a non-empty
   palette, so the palette *length* alone decides the space (the raw id byte
   layout is confirmed by the loaded tail bytes, e.g.
   `… ff ff ff ff 18 00 00 00 24 01 02 05 …` = id `-1`, count `24`, then the
   palette). The id is otherwise an engine matrix-map registration value
   (`-2..-6` and `0..4` all shipped) and is recorded but not interpreted.
3. **Decisive evidence** (`Riddick_B05.xmd`, cluster 1, the left arm): raw
   indices `0..23` remap through
   `matrix_bones = [43,42,41,16,44,38,45,14,39,47,50,48,51,46,40,49,52,13,12,9,8,1,17,18]`
   to nodes `{38..52, 16, 14, 13, 12, 9, 8, 1, 17, 18}` — exactly the left-arm
   chain (node 12 shoulder → 13 → 14 elbow → 16 wrist) plus the five **finger**
   chains (nodes 38..52, bind `y ≈ -20.4..-24.9`). Grouping the cluster's
   vertices by their dominant remapped bone places the fingertip geometry
   (`y ≈ -24..-25.4`) on the finger nodes. Read as *direct* node indices those
   same vertices would bind to pelvis/leg bones (`z 0..32`) while the geometry
   is at `z 45..52` — impossible. Its unskinned shadow body (cluster 6) has an
   **empty** palette and uses node indices `0..69` directly, matching the whole
   skeleton.

### Aggregate evidence (all 631 shipped models / 4662 clusters / 3103 skinned)
| property | value |
|---|---|
| paletted clusters (non-empty `matrix_bones`) | **2411** |
| direct clusters (empty palette) | **692** |
| paletted with `len(palette) == max_bone + 1` | **2402** (9 have unused entries) |
| paletted with `max_bone >= len(palette)` | **0** |
| palette entry `>= NODES.count` | **0** |
| palette with duplicate entries | **0** |
| direct with `max_bone >= NODES.count` | **0** |
| max palette length | **24** |
| `matrix_map_id` (paletted) | `-6:1 -5:2 -4:6 -3:2 -2:1166 -1:33 0:721 1:292 2:137 3:50 4:1` |
| `matrix_map_id` (direct) | `-1:692` |

All of the above run inside `mos_xmd.py --check` (631/631 `ok`).

## Change
- `tools/mos_xmd.py`: `BoneDeform.has_palette()`, `.bone_space()`
  (`"palette"`/`"node"`), `.resolve_bone(index)`, `.resolved_influences_of()`,
  `.max_resolved_bone()`, `.used_palette_indices()`, `.palette_fully_used()`;
  `as_dict` carries `bone_space` / `max_resolved_bone` / `matrix_bones` /
  `palette_fully_used`. `cluster_meshes()` records `bone_space` /
  `max_resolved_bone` / `palette_fully_used`. `validate()` now checks the
  **space-correct** invariants (palette index `< len(palette)`, resolved node
  `< NODES.count`, palette entries `< NODES.count`, no duplicates; direct node
  `< NODES.count`). `geometry_stats()` adds `paletted_bone_clusters`,
  `direct_bone_clusters`, `palette_unused_clusters`; `--mesh`/`--cluster` print
  the space, resolved max node, and the palette.
- `tools/xmd_export.py`: `bones_used()` now returns **resolved node** indices;
  `cluster_skin_dict()` gains `bone_space` / `max_bone` / `max_resolved_bone`
  and a `resolved_influences` array alongside the raw `influences`; the manifest
  records `bone_space`.
- `tools/xmd_bake.py`: the baked asset manifest records `bone_space` and
  resolved `bones_used`.
- `tools/test_mos_xmd.py`: rewrote the matrix-map test to a valid palette and
  added palette-space, direct-space, `-1`-with-palette, palette-index-out-of-
  range, palette-entry-out-of-range, duplicate-entry and unused-entry tests.
- `tools/test_xmd_export.py`: added a palette manifest/skin round-trip test.
- Regenerated `docs/rea/reference/native_hand_mesh.json`,
  `riddick_b05_export.json`, `pa2_hand_01_export.json` (resolved bones + space).
- ADR-0417 (layout line + verification) and `docs/rea/mos-xmd-format.md`
  corrected; ADR-0419 bones-used table annotated.

## Evidence (2026-10-16, read-only)
- `python tools/mos_xmd.py --inventory <game>\Content\Models --check` →
  **631/631 `ok`, 0 failures** with the space-correct skinning checks.
- Aggregate via the parser API → **4662** clusters, **3103 skinned**
  (**2411** paletted / **692** direct), **1,315,626** influences, **834,041**
  skinned vertex records; the table above (0 violations on every invariant).
- `python tools/test_mos_xmd.py` **PASS** (40 tests);
  `tools/test_xmd_export.py` **PASS**; `tools/test_xmd_bake.py` **PASS**;
  `python tools/xmd_bake.py --verify src/hook/native_hand_mesh_data.h` → the
  committed header still matches a fresh bake (the generated bind-pose geometry
  bytes are unchanged; only the manifest/comment bookkeeping is affected).

## Consequences
- Skin weights now resolve to the **correct** skeleton nodes, so a consumer can
  bind the real arm/hand mesh (nodes 38..52 = fingers, 16 = wrist, 14 = elbow,
  13/12 = upper arm/shoulder) instead of the pelvis/leg bones. This is the
  needed half of the pose pipeline for GH #46 alongside the `.xsa` clip decode.
- The earlier "bones used" figures in ADR-0419/0420 were **palette indices**;
  the resolved node sets differ (e.g. `Riddick_B05` left arm = nodes
  `{1,8,9,12,13,14,16,17,18,38..52}`).
- **Still open:** the model-space pose is *not* the `NODES` bind translation —
  the arm mesh sits at `z ≈ 45..52` while the arm bones' bind translations run
  `z 48.8 → 36.8`. The mesh is therefore a reference pose distinct from the
  `NODES` bind pose; a correct skinned draw still needs the animation pose
  (ADR-0422) plus the pose/base pairing. The `matrix_map_id` semantics and the
  `MATRIXMAP`-style shared-registry relationship remain uninterpreted.

## Reproduce
```
python tools/mos_xmd.py <game>\Content\Models\Characters\Riddick\Riddick_B05.xmd --cluster 1
python tools/mos_xmd.py --inventory <game>\Content\Models --check
python tools/test_mos_xmd.py
```
