# ADR 0427: native hand/arm draw path behind a config flag

## Status
**Accepted (provisional, default OFF)** — the baked native Riddick arm/hand mesh
(`native_hand_mesh_data.h`) is now consumed by `VRHandRenderer` through a
headset-tunable placement, gated by the `[VR] NativeHands` INI key. The
procedural overlay remains the default and the fallback. No game file is
modified and no binary is deployed.

This is the **draw-path** step of GH #46, following the container/geometry/skin
recovery (#55: ADR-0414..0421), the pose/bind analysis (ADR-0424/0425), the
`xmd_bake` asset (ADR-0420) and the `BONEMATRIXMAP`/palette identification
(ADR-0423/0426). It is deliberately the *smallest* consumer of that work: a
rigid, single-cluster draw with a provisional placement, not a skinned draw.

## Context
Ten wakes of read-only RE produced a baked, compile-in asset
(`src/hook/native_hand_mesh.h`, 4 meshes / 1548 vertices) but nothing in the
hook used it — the hook still drew the procedural `VRHandRenderer` overlay, so
users still saw the wrong (non-game) hands (GH #46). Two facts from the earlier
wakes constrain any first draw:

- **Observed (`native_hand_mesh_data.h`):** each mesh carries its model-space
  AABB. The `Riddick_LeftArm`/`Riddick_RightArm` clusters are ~20 model units
  long on **model Y** (`LEFT y -25.45..-5.49`, `RIGHT y 17.61..25.45`) — far
  longer than X (~8) or Z (~8) — with the hand at the **-Y** end on the left and
  the **+Y** end on the right (the left-arm fingertips, `y ≈ -24..-25.4`, are the
  cluster's minimum-Y vertices; ADR-0420/0424).
- **Observed (ADR-0424):** the stored geometry is **not** the `NODES` bind pose
  (the arm clusters carry a baked raised pose), and no per-cluster
  reference/bind-inverse transform is stored in the `.xmd` (ADR-0425/0426). So a
  *correct* skinned draw still needs the runtime pose pipeline that #59 is
  chasing; a **rigid** draw of the already-raised arm clusters is the only
  honest step available now.

## Finding / decision
The model-space → hand-local mapping is a **provisional, documented inference**,
not a recovered engine transform:

| hand-local axis | model direction | source |
|---|---|---|
| `+X` (right) | `up × backward` (right-handed) | derived |
| `+Y` (up) | model `+Z` (`.xmd` render space is Z-up, ADR-0415) | observed convention |
| `+Z` (back toward the forearm) | `+Y` (left) / `-Y` (right) — the arm's long axis, sign set so the **hand end** is forward | observed AABB asymmetry |
| origin | the **hand end** = the AABB extreme on the hand side | observed |
| scale | `WorldScale × NativeHandScale` (model units → metres) | INI, default `0.033` |

- The long axis is taken as the AABB's **largest extent** (model Y for both arm
  meshes and both `Pa2_Hand` meshes), so the rule generalizes beyond a hardcoded
  axis. The up axis is model `+Z` unless the arm runs along Z.
- Anchor is the **hand end**, not the AABB centre, so the mesh hangs *back*
  (local `+Z`) from the tracked wrist exactly like the procedural forearm.
- Everything tunable is exposed: `NativeHands` (0/1),
  `NativeHandScale`, `NativeHandOffset{X,Y,Z}` (hand-local metres).

The renderer path:
- `VRHandRenderer::DrawNativeHand(side)` resolves `Riddick_LeftArm` /
  `Riddick_RightArm`, computes the placement, and emits the baked triangles with
  `glBegin(GL_TRIANGLES)` / `glNormal3f` / `glVertex3f`, honouring the same
  per-hand fade and the state save/restore `RenderHands` already performs.
- When enabled it replaces the procedural gauntlet/palm/digits and the
  elbow→wrist cloth segment; the interaction glow visuals and the upper-arm
  segment are kept. When disabled (or the mesh/placement is missing) the
  procedural primitives draw unchanged.

### Explicitly not claimed
- **Not a validated fit.** No headset was used; the scale/offset defaults are a
  starting point and a human must calibrate them. No test asserts the mesh is
  visually correct — only that the mapping is well-formed.
- **Not skinned.** Each arm draws as one rigid cluster; the baked raised pose is
  what ships. Per-bone posing/IK is still #58/#59/#54.
- **Not the game's first-person hands.** These are the shipped *model* clusters
  (`RIDDICK_ARMSCAR_*` surface ids per ADR-0420), not the engine's viewmodel
  draw, which the hook cannot yet intercept.

## Change
- `src/hook/native_hand_pose.h` (new, header-only, dependency-free):
  `NativeHandPlacement`, `ComputeNativeHandPlacement`, `MapNativeHandPoint`,
  `MapNativeHandNormal` (AABB → hand-local basis + anchor + scale).
- `src/hook/vr_hand_renderer.h/.cpp`: `DrawNativeHand(HandSide)`; gated
  replacement of the procedural hand/forearm; keeps `VRHandRenderer` as the
  default/fallback.
- `src/hook/vr_config.h/.cpp`: `nativeHands`, `nativeHandScale`,
  `nativeHandOffset{X,Y,Z}` with clamped INI parsing.
- `tests/native_hand_pose_tests.cpp` (new, CTest `native_hand_pose_mapping`):
  orthonormal/right-handed basis, left/right mirror, hand-end → origin,
  scaled span, normal mapping, degenerate/inverted/non-positive-scale rejection,
  and a baked-asset cross-check (arm meshes are Y-longest; anchors pick the
  correct hand end).
- `CMakeLists.txt` (new test target + headers in the hook target),
  `tools/riddick_vr.ini.example`, `riddick_vr.ini`.

## Evidence (2026-10-16, read-only)
- `cmake --build build/x86 --config Release` → **opengl32.dll** built;
  `ctest --test-dir build/x86 -C Release` → **5/5 PASS** (new
  `native_hand_pose_mapping`); same on **build/x64** (host + 5 tests).
- `python tools/test_mos_xmd.py` (50), `test_xmd_export.py`, `test_xmd_bake.py`,
  `test_xmd_pose.py`, `test_xsa_anim.py` (19), `validate_addresses.py`,
  `verify_hook_init.py`, `verify_reported_fixes.py`, `scenario_tests.py --check`
  all PASS.
- Bounds asserted here come from the committed manifest
  (`docs/rea/reference/native_hand_mesh.json`) and the generated header.

## Consequences
- GH #46 has a **switchable** native path; users (or a dev) can set
  `NativeHands=1` and tune `NativeHandScale`/offsets on a headset without a
  rebuild. Default stays procedural, so nothing regresses.
- The placement inference is isolated in one tested header, so when the runtime
  pose (or a recovered bind transform, #59) lands, only
  `ComputeNativeHandPlacement` and the draw loop change.
- Next: calibrate on hardware; then feed the resolved palette→node skin
  (ADR-0423) and the `.xsa` pose (#58) into a skinned variant.

## Files
- `src/hook/native_hand_pose.h`, `src/hook/vr_hand_renderer.{h,cpp}`,
  `src/hook/vr_config.{h,cpp}`, `tests/native_hand_pose_tests.cpp`,
  `CMakeLists.txt`, `tools/riddick_vr.ini.example`, `riddick_vr.ini`
- cross-refs: ADR-0414..0426, GH #46/#55/#58/#59
