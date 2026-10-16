# ADR 0444: bake the native-hand long axis; correct the "file geometry is the bind pose" claim

## Status
**Accepted** — the native-hand draw path now takes the mesh's long (arm) axis
from the **asset** (`NativeHandMesh::arm_axis`, baked from the mesh's *file*
geometry by `tools/xmd_bake.py`) instead of re-deriving it from the mesh's AABB
at draw time. This fixes a demonstrated defect: with the GH #58 posed variant
enabled, the right arm's AABB becomes nearly cubic, the largest-extent rule flips
the axis to model Z, and the right hand is drawn rotated ~90° about the
longitudinal axis and anchored at the wrong end. The change also corrects the
record: the "the file geometry IS the `NODES` bind pose" claim added in ADR-0443
is unsupported and contradicted by ADR-0424's direct measurement. No game file is
modified and no binary is deployed.

## Context
Two independent threads met here.

1. **Placement (GH #64).** `native_hand_pose.h` built the hand-local basis from
   the mesh's model-space AABB, taking the arm's long axis as the **largest AABB
   extent** (ADR-0427). ADR-0427 chose that so the rule "generalizes beyond a
   hardcoded axis". It is fine for the bind geometry but not for a *deformed*
   one. Measured on the committed asset (reproduce below), with the ADR-0443
   pose the right arm's AABB changes from `x 4.82 / y 7.84 / z 6.29` to
   `x 7.93 / y 7.82 / z 8.25` — the largest extent becomes **Z (8.25)**, not Y.
   The placement then picks `up = model Y`, `backward = -model Z`, and anchors at
   the `z = 54.61` extreme instead of the hand end, so the two hands are no longer
   mirror images (left keeps `x-`, `y+` basis; right becomes `x-`, no `y+`) and
   the right hand is grossly mis-placed. A 4% extent difference silently flips the
   whole frame.

2. **Provenance (GH #58/#64).** ADR-0443 (and the `native_hand_mesh.h` comment it
   added) stated the baked geometry "**is** the `NODES` bind pose", citing
   `tools/xmd_fk.py --check`. That citation is invalid: at the bind pose every FK
   world map is the identity, so the influence-weighted blend equals
   `position * sum(weights)` **for any input geometry**, and `pose_drift_max` is
   identically zero whether or not the cluster is stored in the bind pose. The
   `--check` invariant tests the *recipe*, not the *content*. ADR-0424's direct
   measurement stands: on the Riddick arms' shared bones the per-bone geometry
   offsets differ from the body cluster's by **26.6 / 27.3** units (vs **0.93**
   for legs↔body) and the arm geometry AABBs are disjoint from the body — a raised
   reference pose.

The two threads are independent: the ADR-0441 pose recipe stays faithful (it
mirrors the engine, `FUN_1001f340`, which applies the same bind-relative world
deltas to the *stored* geometry), and the fix for thread 1 is orthogonal to
thread 2. Only the wording of thread 2 is corrected.

## Decision
1. `tools/xmd_bake.py` bakes `NativeHandMesh::arm_axis` — the model axis
   (`0 = X`, `1 = Y`, `2 = Z`) of the mesh's long direction, taken from the
   **file** geometry AABB (`_long_axis`, ties → lowest index). It is a property of
   the mesh, so it is stable across the bind and posed variants. All four
   canonical meshes bake to `1` (Y).
2. `native_hand_pose.h` gains `ComputeNativeHandPlacementAlongAxis(min, max,
   isLeft, long_axis, meters_per_unit)`; `ComputeNativeHandPlacement` keeps its
   signature and now just selects the largest extent and delegates (behaviour for
   callers that only have an AABB is unchanged).
3. `DrawNativeHand` calls the axis form with `mesh->arm_axis` when it is `0..2`,
   else the legacy heuristic.
4. The record is corrected: `native_hand_mesh.h`, the `xmd_bake.py` docstring and
   `xmd_fk.py`'s module/`check()` docs now state the tautology and defer to
   ADR-0424; a **new** test pins the tautology so it cannot be re-asserted.

## Evidence (2026-10-16, read-only; 2004 EFBB install)
- **The flip is real.** Applying the ADR-0427 rule to the committed AABBs gives,
  for the *posed* meshes: left `long_axis = Y`, basis `X(-1,0,0) Y(0,0,1)
  Z(0,1,0)`, anchor `y = -26.07`; right `long_axis = Z`, basis `X(-1,0,0)
  Y(0,1,0) Z(0,0,-1)`, anchor `z = 54.61`. The bind right arm is `long_axis = Y`,
  basis `X(1,0,0) Y(0,0,1) Z(0,-1,0)`. (Extents: right posed `7.93 / 7.82 /
  8.245`.)
- **With the baked axis** both hands, bind and posed, use `X = ±model X`,
  `Y = model Z`, `Z = ±model Y` — mirror-symmetric and pose-stable. New CTest
  `native_hand_pose_mapping` asserts exactly this for both arms, both AABBs.
- **The tautology is real.** New test `test_xmd_fk.py::test_bind_check_is_tautological`
  shifts a cluster's geometry far from the bones' bind positions (every vertex
  `> 5` units from the nearest bone bind) and shows `skin_report(...)["pose_drift_max"]
  < 1e-12`, i.e. `xmd_fk --check` would **not** flag it.
- **ADR-0424 reproduced** via `python tools/xmd_pose.py --model
  <game>\Content\Models\Characters\Riddick\Riddick_B05.xmd`: relations
  `c1↔c6 = 26.582`, `c2↔c6 = 27.273`, `c0↔c6 = 0.931`, arm/body AABBs disjoint,
  duplicate pairs `c1↔c4`, `c2↔c5` = 0.000.
- **Rebaked asset**: `python tools/xmd_bake.py --cpp
  src/hook/native_hand_mesh_data.h --manifest
  docs/rea/reference/native_hand_mesh.json` → 4 meshes / 1548 verts / 2610 tris +
  870 posed; `--verify` OK; `arm_axis` recorded in the header and manifest.
- `tools/build-all.ps1 -Config Release` OK (x86 `opengl32.dll` + x64 host);
  CTest **9/9 PASS** on x86 **and** x64; all 8 `tools/test_*.py`,
  `validate_addresses.py`, `verify_hook_init.py`, `verify_reported_fixes.py`
  (new GH #64 axis invariant) and `scenario_tests.py --check` PASS.

## Explicitly not claimed
- **Not a validated fit.** No headset was used; `arm_axis` fixes the frame's
  *stability and symmetry*, not its absolute orientation. `NativeHandYaw/Pitch/Roll`
  and the scale/offsets still need on-headset tuning. `NativeHands`/`NativeHandPose`
  remain off by default.
- **Not the missing per-cluster reference transform.** ADR-0424's raised
  reference pose is left as an open question (GH #59); this ADR only stops
  mis-stating it and makes the draw robust to it.
- **`arm_axis` is a convention**, derived from the file AABB's largest extent; for
  meshes that are not arms it is still just "the long axis". It is baked, so a
  future re-bake with a different rule is a deliberate change.

## Reproduce
```
python tools/xmd_pose.py --model "<game>\Content\Models\Characters\Riddick\Riddick_B05.xmd"
python tools/test_xmd_fk.py
python tools/test_xmd_bake.py
python tools/xmd_bake.py --cpp src/hook/native_hand_mesh_data.h \
                         --manifest docs/rea/reference/native_hand_mesh.json
python tools/xmd_bake.py --cpp src/hook/native_hand_mesh_data.h --verify
```

## Files
- `tools/xmd_bake.py`, `tools/test_xmd_bake.py`, `tools/xmd_fk.py`,
  `tools/test_xmd_fk.py`, `src/hook/native_hand_mesh.h`,
  `src/hook/native_hand_mesh_data.h` (regenerated), `src/hook/native_hand_pose.h`,
  `src/hook/vr_hand_renderer.cpp`, `tests/native_hand_pose_tests.cpp`,
  `tests/native_hand_mesh_tests.cpp`, `tools/verify_reported_fixes.py`,
  `docs/rea/reference/native_hand_mesh.json`
- cross-refs: ADR-0420/0424/0427/0436/0441/0442/0443; GH #46/#58/#64
