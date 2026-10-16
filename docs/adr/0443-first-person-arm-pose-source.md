# ADR 0443: first-person arm pose source — offline `.xsa` pose baked into the native hand asset

## Status
**Accepted (tooling + asset + opt-in draw path)** — the first-person arm/hand
**pose source** for the native draw is pinned to the engine's own first-person
animation clips (`Content/Anim/Main/FPS*.xsa`), and the ADR-0441/0442 offline FK
evaluator is wired into `tools/xmd_bake.py` so a posed variant of the baked
Riddick arm clusters is compiled into `native_hand_mesh_data.h`. The draw path
consumes the posed variant only behind `[VR] NativeHandPose` (default **off**);
the bind geometry and the procedural overlay remain the fallbacks.

This closes the "pick the first-person arm pose source" next-step of ADR-0442 /
GH #58. It does **not** claim a validated VR fit (no headset) and does **not**
close GH #34 (runtime matrix read), which the offline path no longer needs.

## Context
Three facts were in hand before this ADR:

- `.xmd` stores **no bone rotations** (ADR-0421); the rotation source had to be
  the `.xsa`/`.xag` animation groups or the engine's live skeleton (GH #58).
- `.xsa` is decoded (ADR-0422) and the ADR-0441 FK/skinning algebra is
  implemented offline (`tools/xmd_fk.py`, ADR-0442), verified over the whole
  shipped corpus.
- The native hand asset (ADR-0420) is baked from `Riddick_B05.xmd` clusters 1/2
  in the **bind pose**, which holds the arms out along ±Y (a T-pose-like rest),
  so the drawn hand does not read as a first-person arm — GH #64's "out of
  place".

What was missing was (a) **which** shipped animation drives Riddick's
first-person arms, and (b) a way to turn that into asset data so the mesh can be
drawn posed.

## Finding 1 — the pose source is the `FPS*.xsa` clips
`Content/Anim/Main/` ships exactly five first-person animation sets:

| clip | sequences | rot/mov window |
|---|---|---|
| `FPSAssault.xsa` | `StdAim_Up/Dwn`, `ChrAim_Up/Dwn`, `FPSAssHit`, `mhstd_ass_aim_close`, `mh_chr_ass_aim_close` | 76 / 19 |
| `FPSPistol.xsa`  | `FPSPisHit`, `Pis_MhStd_close`, `PisChr_MhChr_close`, `Pis_MhStd_Up`, `Pis_MhStd_Dwn` | 76 / 76 |
| `FPSMini.xsa`    | `Min_StdClo`, `Min_SteClo`, `Min_Equip`, `Min_UnEquip`, `Min_SteEquip`, `Min_SteUnEquip` | 71 / 71 |
| `FPSGrenade.xsa` | `Gren_MhStd`, `Gren_Equip`, `Gren_UnEquip`, `Gren_MelHit`, `Gren_Throw` | 72 / 11 |
| `FPSMelee.xsa`   | `Mel_ShaEquip/UnEquip`, `Mel_CluEquip/UnEquip` | 71 / 10 |

Decisive cross-check (read-only, `tools/xsa_anim.py` + `tools/xmd_fk.py`):
`Riddick_B05.xmd` carries a 70-node `NODES` skeleton with rotation slots
`0..69` and movement slots `0..18`. Every `FPS*` clip's rotation window covers
slots `0..69` (window ≥ 71), and posing the model from
`FPSPistol:Pis_MhStd_Dwn` resolves **all** node slots with **no mapping notes**
(`pose_from_sequence` returns an empty `notes` list). The clips therefore drive
the *same* Riddick skeleton the baked arm clusters are skinned to (bones
`[1,8,9,12,13,14,16,17,18,38..52]` for cluster 1, `[9,17..19,21,53..67]` for
cluster 2) — no separate first-person skeleton/model exists under
`Content/Models/Characters/Riddick/`.

## Finding 2 — an "at rest" frame exists
The clips are all actions (aim up/down, hit, equip/unequip, throw) — none is a
plain idle. The **unequip** clips return the hands from a held weapon to rest,
so their **last** key is the most neutral shipped first-person frame. Posing
`FPSMelee:Mel_ShaUnEquip` at its last key (`KEYFRAMETIMES[17] = 0.3334 s`) moves
the left arm only ≈10.5 model units and the right ≈12.0 (vs. 25–42 units for the
aim poses), i.e. it is the closest to the bind rest while still being a real
in-engine first-person pose.

## Decision
1. `tools/xmd_bake.py` gains `--anim/--sequence/--pose-key` (and `--no-pose`).
   For every **skinned** cluster it evaluates the ADR-0441 recipe offline via
   `tools/xmd_fk.py` — FK over `NODES` from the clip's window slots, then the
   influence-weighted skin blend — and emits a `_Posed_Vertices[]` array plus the
   posed AABB and the clip provenance (name, sha256, size, time) into
   `src/hook/native_hand_mesh_data.h`.
2. The canonical pose is pinned **by KEYFRAMETIMES index**, not a float, so the
   bake is deterministic: `FPSMelee.xsa :: Mel_ShaUnEquip :: key 17`.
3. `NativeHandMesh` gains optional `posed_vertices` / `pose_source` /
   `posed_bounds_min/max` and `has_pose()`; unskinned clusters and any cluster
   without a baked pose carry `nullptr` + `""`.
4. `DrawNativeHand` selects the posed arrays **and** the posed AABB when
   `[VR] NativeHandPose=1` and a pose exists, otherwise the bind geometry. New
   INI key `NativeHandPose`, default **0**.
5. Unit tests: `tests/native_hand_mesh_tests.cpp` (posed topology/bounds/finite
   checks, arms must carry a pose that differs >1 unit from bind, unskinned
   clusters must not) and `tools/test_xmd_bake.py` (the emitted pose must equal
   `xmd_fk`'s skinning of the same clip/key to 1e-9; unskinned skip; CLI +
   `--verify`; `--no-pose`).

## Evidence (2026-10-16, read-only; 2004 EFBB install)
- `python tools/xmd_bake.py --cpp src/hook/native_hand_mesh_data.h
  --manifest docs/rea/reference/native_hand_mesh.json` → 4 meshes / 1548 verts /
  2610 tris + **870 posed vertices** (526 + 344 = the two Riddick arms);
  `Pa2_Hand*` are unskinned (no skeleton) and are correctly left unposed.
- Pose provenance recorded in the header and the manifest:
  `FPSMelee.xsa:Mel_ShaUnEquip@key17=0.333400s`, clip
  `sha256 d6fbb681…c6ef7e`, size 21068.
- Deformations (model units, `xmd_fk.cluster_analysis`): left `displacement_max`
  10.47, right 12.01; `Pis_MhStd_Dwn` (a weapon pose) gives 27.3 / 34.9 and
  `Min_StdClo` 32.6 / 42.2 — the unequip frame is the neutral choice.
- `python tools/test_xmd_bake.py` → 10 tests PASS (incl. 4 new posed-bake tests).
- `python tools/test_xmd_fk.py`, `tools/test_xsa_anim.py`, `tools/test_mos_xmd.py`,
  `tools/test_xmd_export.py` unaffected / PASS.
- Build + CTest: `tools/build-all.ps1 -Config Release` OK (x86 `opengl32.dll` +
  x64 host); CTest x86 **and** x64 green (incl. `native_hand_mesh_asset` with the
  new pose assertions).

## Explicitly not claimed
- **Not a validated VR fit.** No headset was used. `NativeHandPose` defaults to
  0; the pose is a real engine first-person frame, not a fit to a tracked
  controller.
- **Not a runtime-animated hand.** The pose is baked **statically**. Following
  the engine's live per-frame pose is GH #34 (runtime read) — the offline path
  and the runtime read are now independently comparable.
- **The placement is unchanged.** `DrawNativeHand` still derives the
  model→hand-local placement from the (now posed) AABB with the provisional
  ADR-0427 basis; it is not an authoritative hand-bone frame. The bind pose
  carries no bone *orientation* at all (ADR-0441: every bind world matrix is the
  identity), so no absolute hand-bone basis can be derived offline from this
  data.
- **`.xsa` sampling is the nlerp/lerp approximation** (ADR-0422), not the
  engine's Hermite spline; and the QUATERNIONS-pool movement slot
  (`TRACK_FLAG_MOVEMENT`) is reported as zero translation, not guessed.
- **`FPSMelee:Mel_ShaUnEquip` is a reproducible choice, not the "correct" one.**
  Re-bake any other clip with `--anim/--sequence/--pose-key`.

## Update (GH #64, ADR-0444)
Two corrections:

- **"Bind pose" wording.** The asset is baked from the *file* geometry, which is
  **not** the `NODES` bind pose for the arm clusters (ADR-0424 measured ~26-27
  unit same-bone offsets vs the body; the arms are a raised reference pose). The
  phrase "in the bind pose" above should read "in the file (raised-reference)
  pose". This does **not** invalidate the ADR-0441 pose recipe, which mirrors the
  engine and applies the same bind-relative world deltas to the stored geometry.
- **Placement.** `DrawNativeHand` no longer derives the model→hand-local basis
  from the (posed) AABB's largest extent — that flipped for the posed right arm.
  It now uses the mesh's baked `arm_axis` (native_hand_mesh.h), so both hands are
  mirror-symmetric and pose-stable. See ADR-0444.

## Consequences
- GH #58's pose source is pinned and the asset now carries a real first-person
  pose; `NativeHandPose=1` is a one-key headset A/B against the bind geometry.
- Re-baking a different clip/weapon pose is a tool invocation, not a code change.
- The draw remains honest: the procedural overlay is still the default and the
  bind asset is still the default native geometry.

## Files
- `tools/xmd_bake.py`, `tools/test_xmd_bake.py`,
  `src/hook/native_hand_mesh.h`, `src/hook/native_hand_mesh_data.h` (regenerated),
  `src/hook/vr_hand_renderer.cpp`, `src/hook/vr_config.{h,cpp}`,
  `tests/native_hand_mesh_tests.cpp`, `riddick_vr.ini`,
  `tools/riddick_vr.ini.example`, `src/hook/README.md`,
  `tools/verify_reported_fixes.py`, `docs/rea/reference/native_hand_mesh.json`
- cross-refs: ADR-0418/0420/0421/0422/0423/0427/0436/0441/0442; GH #46/#55/#58/#64
