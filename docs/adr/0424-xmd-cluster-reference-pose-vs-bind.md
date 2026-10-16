# ADR 0424: `.xmd` clusters carry a per-cluster reference pose, not the `NODES` bind pose

## Status
**Accepted** — a `.xmd` cluster's stored `VERTEXFRAMES` geometry is not, in
general, in the `NODES` bind pose: the shipped `Riddick_B05.xmd` stores its body
and legs clusters in one reference pose and its two arm clusters in a different
(raised) pose. Measured read-only across the 268 shipped models / 3454 clusters
(3103 skinned) by the new `tools/xmd_pose.py`; no game file is modified and no
code is deployed.

This is the **pose/base pairing** question left open by ADR-0422 (rotation
source for the native draw path) and ADR-0423 (skin weights resolve to correct
skeleton nodes). It is a prerequisite for a **skinned** native draw path
(GH #46 / GH #55): the "bind pose" a skin matrix must invert is per cluster, not
the single `NODES` array.

## Context
ADR-0417/0419 recovered the geometry, skin weights and skeleton; ADR-0422/0423
recovered the `.xsa` rotations and the `matrix_bones` palette. Both closed on the
assumption that a cluster's stored geometry is the artist bind pose, so that the
`NODES` translations *are* the bind. ADR-0423 closed with an explicit caveat that
this had not been checked ("the model-space pose is *not* the `NODES` bind
translation"). This ADR checks it.

## Method
`tools/xmd_pose.py` computes, per cluster, the **weighted geometry centroid** of
every skeleton node the cluster influences (weights resolved through the
`matrix_bones` palette, ADR-0423), and the **bind offset** `centroid −
NODES.translation`. It then compares two clusters on their shared bones: if their
per-bone bind offsets agree for every shared bone, the clusters are stored in the
same reference pose; if they disagree, they are not. It also reports the
geometry AABB against the AABB of the bind positions of the bones the cluster
uses.

This is deliberately a *diagnostic*: it reports the pairing instead of assuming
one. The weighted centroid is a coarse proxy for a bone's model-space position
(geometry spreads around a joint), so the useful signal is the **difference**
between two clusters on the *same* bone, which cancels the joint-vs-geometry
bias.

## Finding (Riddick_B05.xmd — 70 nodes, 7 clusters; read-only)
Cluster geometry Z ranges (engine model space, Z-up):

| cluster | part | verts | geometry Z |
|---|---|---|---|
| 0 / 3 | legs (duplicate, empty `TVERTEXFRAMES`) | 372 | 0.00 .. 25.04 |
| 1 / 4 | left arm (textured `RIDDICK_ARMSCAR_*`) | 526 | 45.15 .. 52.50 |
| 2 / 5 | right arm | 344 | 44.22 .. 50.52 |
| 6 | shadow body | 956 | 0.00 .. 36.26 |

`tools/xmd_pose.py --model Riddick_B05.xmd` relations (per-bone bind-offset
agreement over the shared bones):

| relation | shared bones | centroid Δ rms | Δ max | AABBs overlap |
|---|---|---|---|---|
| c0 ↔ c6 (legs vs body) | 10 | **0.931** | 2.69 | **yes** |
| c1 ↔ c6 (left arm vs body) | 24 | **26.58** | 29.02 | **no** |
| c2 ↔ c6 (right arm vs body) | 20 | **27.27** | 28.36 | **no** |
| c0 ↔ c3, c1 ↔ c4, c2 ↔ c5 | 10 / 24 / 20 | 0.0 | 0.0 | yes |

1. **The legs cluster and the body cluster are the same reference pose.** Their
   10 shared leg bones agree to under **1 unit** on a ~36-unit figure (rms 0.93,
   max 2.69), and their AABBs overlap. Observation.
2. **The arm clusters are a different reference pose.** On their 24 / 20 shared
   arm bones the per-bone offsets differ by **~26–29 units** and their geometry
   AABBs are **disjoint** from the body: the left-arm cluster's lowest Z (45.15)
   is above the body cluster's highest Z (36.26). Observation.
3. **Neither is simply the `NODES` bind pose.** The `NODES` bind positions of the
   arm bones span Z `33.60 .. 48.78`, i.e. they overlap the body (Z `0..36`),
   while the arm *geometry* is entirely above it. So the arm clusters carry a
   large baked rotation relative to the body and to the bind skeleton.
   Observation.
4. **Inference (not measured):** the arm clusters are stored in a raised,
   chest-height "ready"/first-person pose, which is why their geometry sits at
   shoulder height and reaches forward instead of hanging at the body's side.
   The exact per-bone transform that reconciles a cluster with the `NODES` bind
   (most plausibly resolved through the `matrix_map_id` / engine per-cluster
   matrix setup) is **not** recovered here.

Corpus (read-only, `tools/xmd_pose.py --inventory <game>\Content\Models --check`):
**268 models, 3454 clusters, 3103 skinned, 0 problems** — every skinned cluster
has positive total weight, all centroids/offsets are finite, and every influence
resolves to a used `NODES` node.

## Change
- `tools/xmd_pose.py` (new): `bone_centroids`, `bind_table`, `cluster_pose`,
  `cluster_relation`, `analyze`, `check`; CLI `--model`/`--cluster`/`--relation`/
  `--bind-table`/`--offsets`/`--inventory --check`/`--json`. Read-only,
  dependency-free.
- `tools/test_xmd_pose.py` (new): 7 synthetic-container tests (weighted
  centroids, bind table parent/child/axis/local-offset, same-vs-shifted cluster
  relations, no-shared-bone relation, `analyze`, `check` clean+empty, CLI incl.
  error paths).
- `.github/workflows/vr-worker.yml`: run the new test next to the `.xmd` tests.
- `docs/rea/reference/riddick_b05_pose.json` (new evidence: the bind table +
  per-cluster summaries + relations).
- `docs/rea/mos-xmd-format.md`: `.xmd` reference-pose note + "Next steps"
  updated.

## Consequences
- A **skinned** native draw path must not use one global bind pose. The arms
  baked by ADR-0420 are a raised reference pose already, so a **rigid** native
  draw (GH #46's first step) is viable without solving the pairing — which is
  consistent with the plan to ship the rigid draw behind a config flag first.
- The remaining work for animation is a **per-cluster reference transform**
  (bind-inverse) rather than a single `NODES`-derived bind. The `.xsa` clip
  rotations (ADR-0422) sample fine, but they must be composed in the cluster's
  own reference frame.
- `bind_table` emits a renderer-facing bind skeleton (translation, parent,
  children, derived bone axis, local offset) that a native consumer can use
  directly; bone **roll** is not stored in `.xmd` and is derived from the
  child direction, so it is a convention, not recovered data.

## Reproduce
```
python tools/xmd_pose.py --model <game>\Content\Models\Characters\Riddick\Riddick_B05.xmd
python tools/xmd_pose.py --model <game>\Content\Models\Characters\Riddick\Riddick_B05.xmd --relation 1 6
python tools/xmd_pose.py --inventory <game>\Content\Models --check
python tools/test_xmd_pose.py
```

## Files
- `tools/xmd_pose.py`, `tools/test_xmd_pose.py`
- `docs/rea/reference/riddick_b05_pose.json`
- `.github/workflows/vr-worker.yml`
- `docs/rea/mos-xmd-format.md`
- cross-refs: ADR-0417, ADR-0419, ADR-0420, ADR-0421, ADR-0422, ADR-0423
