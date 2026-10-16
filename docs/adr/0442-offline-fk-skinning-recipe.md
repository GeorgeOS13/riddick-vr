# ADR 0442: offline forward-kinematics + skinning recipe for `.xmd` models

## Status
**Accepted (tooling, read-only)** — the forward-kinematics / skinning recipe that
ADR-0441 recovered from `MXR.dll` is now implemented offline and verified over the
whole shipped corpus by `tools/xmd_fk.py` (+ `tools/test_xmd_fk.py`). No C++/hook
change; the native draw (ADR-0427/0436) stays rigid and the procedural overlay
stays the default.

This closes the "Next" of ADR-0440/0441 and the "offline `bind_inverse × .xsa`
reconstruction" half of GH #58. It does **not** close GH #34 (a runtime read of the
live matrix vector) — it removes the need for it on the offline path.

## Context
ADR-0441 pinned the engine algebra: the skin matrix list *is* the skeleton FK
output, the local map is `v ↦ R·(v − bindpos) + bindpos + t` (rotation about the
bone's own `NODES` bind translation, `R`/`t` the `.xsa` deltas from bind), world =
`parent · local`, and a vertex is the influence weighted sum. At bind every world
map is the identity, so the file geometry *is* the bind pose — no bind-inverse, no
load-time rebase. ADR-0422 had already decoded `.xsa` and pinned the
`rotation_slot` / `movement_slot` binding. What was missing was a deterministic
consumer that turns "a clip + a time" into a posed mesh, so the native hand
(GH #46) can be evaluated and tuned offline instead of only on a headset.

## Finding / decision
`tools/xmd_fk.py` implements the recipe as pure, dependency-free math:

| concept | code |
|---|---|
| rotation | `quat_to_mat3((x,y,z,w))`, normalised |
| bone local | `local_from_bind(bindpos, quat, move)` → `(R, bindpos + move − R·bindpos)` |
| tree walk | `world_matrices(skeleton, deltas)` → `world[n] = world[parent] · local[n]` |
| skinning | `skin_positions` / `skin_normals` — influence **weighted sum** (rotation-only for normals) |
| clip → bones | `pose_from_sequence(skeleton, seq, sampler, t)` — the clip's rotation window indexed by `rotation_slot`, its movement window by `movement_slot` |

Two design choices keep it honest:

- **Weight sums are not renormalised.** The engine sums the influence maps
  (`FUN_100254e0`), so the tool does too and *reports* any vertex whose weights do
  not sum to 1 (a data property, not an error).
- **The bind-pose check isolates the pose from the weights.** `pose_drift_max` is
  `|blend − position·Σw|`; at bind it is exactly 0 for any weight sum, so the
  corpus check tests ADR-0441 and not the content's weights. `identity_drift_max`
  (`|blend − position|`) is also reported, for content whose weights do sum to 1.

The movement-window slot whose track routes to the `QUATERNIONS` pool
(`TRACK_FLAG_MOVEMENT`, ADR-0422) has no pinned consumer, so it contributes a zero
translation and is reported in `notes` rather than guessed.

## Evidence (2026-10-16, read-only; 2004 EFBB install)
- **Bind == file geometry, corpus-wide:**
  `python tools/xmd_fk.py --inventory "<game>\Content\Models" --check` →
  `models=268 clusters=3454 skinned=3103 vertices=915508`,
  `worst bind pose drift=1.42e-14`, `worst weight error=3.92e-3`, **problems: 0**.
  This is ADR-0441's decisive claim made falsifiable where it matters and it holds
  everywhere shipped.
- **Per model** `Riddick_B05.xmd` (7 clusters, 3442 verts): every cluster's bind
  drift ≤ 7.95e-15 (`identity_drift_max`), confirming the file mesh is the bind
  pose.
- **A real clip poses the arms:** `--model Riddick_B05.xmd --cluster 1
  --anim Content\Anim\Main\Use.xsa --sequence use_pad_rid --time 0.2` →
  displacement 12.4 model units on the left arm (≈0.4 m at the 0.033 scale), with
  no mapping notes (all slots in-window). OBJ export written and inspected
  (526 vertices).
- `python tools/test_xmd_fk.py` → **10** synthetic-container tests PASS (quaternion
  / `mat3` (orthonormality, 90°, non-unit, degenerate), local rotate-about-bindpos
  and direct translation, `world = parent · local` accumulation and rotation about
  the root, bind identity end-to-end on a translated skeleton, root-rotation of a
  posed mesh, weighted-sum skinning + weight-outlier reporting, the slot mapping
  incl. the QUATERNIONS-pool movement case, the CLI `--json/--joints/--check/--obj`
  and `--inventory --check`, and the error paths).

## Explicitly not claimed
- **Not a validated fit and not a renderer change.** No headset was used and the
  hook still draws the baked mesh rigidly (ADR-0427/0436). This ADR is the offline
  evaluator, not the draw path.
- **Not the engine's own interpolation.** `.xsa` sampling stays `xsa_anim`'s
  normalised-lerp / linear approximation (ADR-0422); the engine uses a Hermite
  spline.
- **Not the first-person pose source.** Which clip/predicate drives Riddick's
  first-person arms is still unidentified (GH #58), and the runtime matrix vector
  is still unread (GH #34).
- **The QUATERNIONS-pool movement slot is unresolved** (reported, treated as zero).

## Change
- `tools/xmd_fk.py` (new): FK + skinning + clip sampling + OBJ export; `--bind`,
  `--anim/--sequence/--time`, `--joints`, `--obj`, `--check`, `--inventory`,
  `--json`.
- `tools/test_xmd_fk.py` (new): 10 synthetic-container tests (no game install).
- `.github/workflows/vr-worker.yml`: run `test_xmd_fk.py`.
- `docs/rea/mos-xmd-format.md`: an "Implemented offline (ADR-0442)" paragraph after
  the ADR-0441 consequence.

## Consequences
- The native skinned hand no longer needs the runtime read to be *prototyped*: a
  pose + the file mesh + `xmd_fk` reproduce what the engine would draw. The
  remaining unknown is the **pose source**, not the math.
- The offline path and the runtime read (GH #34) are now independently testable
  against each other if the runtime read lands.

## Files
- `tools/xmd_fk.py`, `tools/test_xmd_fk.py`, `.github/workflows/vr-worker.yml`,
  `docs/rea/mos-xmd-format.md`
- cross-refs: ADR-0418/0422/0423/0424/0427/0428/0429/0430/0436/0440/0441;
  GH #34/#46/#55/#58/#59/#60
