# ADR 0425: the `.xmd` `CLUSTER`/`MISCELLANEOUS` header is a reference bounding box, not a matrix

## Status
**Accepted** — the 44-byte `MISCELLANEOUS` record that precedes every
`VERTEXFRAMES` chunk is `{u32 version; u32 flags; u32 surface_index; u32
reserved; f32 box_min[3]; f32 box_max[3]; f32 trailer}`. The final 28 bytes,
previously treated as opaque (ADR-0420), are six floats forming a **reference
bounding box** that *encloses* the cluster's decoded `VERTEXFRAMES` AABB on all
4662 shipped clusters (631 models). Measured read-only; no game file is modified
and no code is deployed.

This closes the "per-`CLUSTER`/`MISCELLANEOUS` matrix" candidate from GH #59:
the record holds a **bound**, not a transform. The per-cluster
reference/bind-inverse transform is therefore *not* stored in the cluster
header.

## Context
ADR-0420 decoded the first 16 bytes of the record (`version`, `flags`,
`surface_index`, `reserved`) and left the trailing 28 bytes unmapped. ADR-0424
then established that a cluster's stored geometry is not, in general, the
`NODES` bind pose, and GH #59 listed "a per-`CLUSTER`/`MISCELLANEOUS` matrix"
as one of three candidate homes for the missing per-cluster reference
transform. This ADR pins what the trailing bytes actually are.

## Record layout (44 bytes, `MISC_RECORD_LEN`)
| off | type | field | shipped value |
|---|---|---|---|
| `+0x00` | u32 | `version` | `0x102` on **every** shipped cluster |
| `+0x04` | u32 | `flags` | `{0: 2099, 1: 839, 3: 1724}` |
| `+0x08` | u32 | `surface_index` | `< SURFACES.count`, covers every surface |
| `+0x0C` | u32 | `reserved` | `0` on every shipped cluster |
| `+0x10` | f32[3] | `box_min` | reference lower corner |
| `+0x1C` | f32[3] | `box_max` | reference upper corner |
| `+0x28` | f32 | `trailer` | `0.0` on every shipped cluster |

The loader `MXR.dll` `CTriangleMeshCore::Read` (`FUN_10080f50`) reads only the
leading `u32` (`MISCELLANEOUS` +0x00) as the model version, which is the same
value the `BONEDEFORM` reader (`FUN_1007d260`) branches on; the remaining bytes
are the cluster's static geometry metadata.

## Finding (read-only, corpus-wide)
`tools/mos_xmd.py --inventory <game>\Content\Models --check` and the reference
box decode:

- **631 models / 4662 clusters**, one `MISCELLANEOUS` per cluster (**631/631**
  count agreement).
- Every record is exactly 44 bytes; `version == 0x102`, `reserved == 0` and
  `trailer == 0.0` on **4662/4662**.
- **The box is exactly the paired `VERTEXFRAMES` frame's quantization range.**
  `(box_min, box_max)` equals the cluster's frame header `(bounds_min,
  bounds_max)` (ADR-0415) component-wise (tolerance `1e-3`) on **4662/4662** —
  `--check` now enforces this. The `MISCELLANEOUS` header therefore *mirrors* the
  cluster's dequantization/reference box; it stores no additional transform.
- Consequently the box encloses the *decoded* geometry AABB on **4662/4662**
  (the quantized positions always map inside `[bounds_min, bounds_max]`), while
  it equals the decoded geometry AABB exactly on only **1751/4662** — i.e. for
  2911 clusters the quantization range is a strict superset of the geometry.

`Riddick_B05.xmd` (70 nodes, 7 clusters) shows both cases; the `MISCELLANEOUS`
box equals the frame's quantization bounds for every cluster:

| cluster | part | decoded geometry AABB Z | reference box (== frame bounds) Z | box == decoded geom |
|---|---|---|---|---|
| 0 / 3 | legs mesh | 0.00 .. 25.04 | 0.00 .. 36.26 | no |
| 1 / 4 | left arm | 45.15 .. 52.50 | 32.85 .. 52.50 | no |
| 2 / 5 | right arm | 44.22 .. 50.52 | 32.85 .. 51.03 | no |
| 6 | shadow body | 0.00 .. 36.26 | 0.00 .. 36.26 | **yes** |

Observations:
1. The `MISCELLANEOUS` box is a **copy of the `VERTEXFRAMES` quantization range**
   — never a transform. The 28 trailing bytes add no information a consumer of
   the frame header does not already have.
2. The quantization range is a **reference box** that contains the decoded
   geometry but is often larger (2911/4662). It is therefore *not* simply the
   cluster's stored-geometry AABB.
3. It is **not** the `NODES`-bind AABB of the cluster's bone set and **not** the
   model `BOUND`: the left-arm box (`y -25.45 .. 6.42`, `z 32.85 .. 52.50`)
   exceeds the bind AABB of the 24 arm nodes it references, while the legs
   cluster's box equals the shadow-body cluster's box (`z 0 .. 36.26`), i.e. the
   whole-body bound rather than the legs' own geometry.

Inference (not measured): the shared quantization/reference box is a
**conservative per-cluster bound** (the region the cluster can occupy once it is
posed into the model's reference/animation space) rather than the stored-geometry
AABB, which is why it is larger for clusters (arms, legs) whose stored geometry
is not the reference pose (ADR-0424). Its exact authoring derivation is **open**;
it is usable as a culling bound but does not give the reference transform.

### Exploratory (not conclusive)
Fitting a single rigid transform (`Kabsch`) from the bind bone positions to the
per-bone geometry centroids leaves residuals of the same order as the centroid
spread (ratio `0.7 .. 2.1`) on `Riddick_B05`/`Riddick_H01`, and a one-parameter
shoulder rotation that best re-anchors the arm clusters runs ~**25–29°** about
the lateral axis (`xmd_pose` centroid proxy, not exact). Both are consistent
with an arm T-pose-geometry / A-pose-bind interpretation but neither proves it,
so no transform is claimed here.

## Change
- `tools/mos_xmd.py`: `ClusterInfo` gains `box_min`/`box_max`/`trailer` and
  `encloses()`; `decode_cluster_info` decodes the six floats + trailer;
  `cluster_meshes()` carries `reference_box_min`/`reference_box_max`/
  `reference_box_matches_frame_bounds`; `validate()` enforces the
  box == `VERTEXFRAMES` quantization-range equality; `geometry_stats()`/
  `geometry_summary()` report `reference_box_clusters`/
  `reference_box_mismatch_clusters`/`reference_box_missing_clusters`; `--mesh`
  prints the reference box; module docstring documents the record.
- `tools/test_mos_xmd.py`: **+4** tests (box decode + `encloses`, box == frame
  bounds, box/frame mismatch rejection, box/trailer decode) → 47 tests.
- `tools/xmd_pose.py`: `analyze()` attaches the reference box and an
  `reference_box_encloses_geometry` flag (decoded geometry AABB) per cluster;
  `check()` fails when a box fails to enclose the decoded geometry; the plain
  text report shows `refbox=`.
- `docs/rea/reference/riddick_b05_pose.json` regenerated (per-cluster reference
  boxes).
- `docs/rea/mos-xmd-format.md`: `MISCELLANEOUS` section.

## Consequences
- **GH #59 narrows:** the per-cluster reference/bind-inverse transform is not in
  the `MISCELLANEOUS` header. The remaining candidates are the `matrix_map_id`
  registry (ADR-0423) or a runtime-per-cluster matrix built by the model/skin
  class.
- `--check` gains a new corpus invariant (box == frame quantization range,
  4662/4662) that guards future parser/decoder changes and pins the redundancy.
- The reference box is available to a native renderer as a ready-made
  per-cluster bound, but a consumer can read it directly from the
  `VERTEXFRAMES` frame header without the `MISCELLANEOUS` mirror.

## Reproduce
```
python tools/mos_xmd.py <game>\Content\Models\Characters\Riddick\Riddick_B05.xmd --mesh
python tools/mos_xmd.py --inventory <game>\Content\Models --check
python tools/xmd_pose.py --model <game>\Content\Models\Characters\Riddick\Riddick_B05.xmd
python tools/test_mos_xmd.py
```

## Files
- `tools/mos_xmd.py`, `tools/test_mos_xmd.py`, `tools/xmd_pose.py`
- `docs/rea/reference/riddick_b05_pose.json`
- `docs/rea/mos-xmd-format.md`
- cross-refs: ADR-0417, ADR-0420, ADR-0423, ADR-0424
