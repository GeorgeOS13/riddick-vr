# ADR 0436: Native Hand Reads Solid 3D, and No Stray Procedural Blocks

## Status
**Accepted.** Addresses GH #64 ("Native hands look flat/2D, are out of place, and
leave stray procedural blocks"). Builds on ADR-0420/0427 (baked mesh + placement)
and ADR-0433 (baked diffuse).

## Context
With `NativeHands=1` the native mesh drew (`VRHandRenderer::DrawNativeHand`) with:

- a **hand-rolled two-sided abs-lambert** — `shade = 0.35 + 0.65*|N·L|` computed
  from the baked normals and multiplied into a flat colour — *on top of* the
  fixed-function `GL_LIGHTING`/`GL_LIGHT0` that `RenderHands` already enables, so
  the surface was shaded twice and the `|N·L|` fold removed the light terminator
  (no dark side), reading as a flat card; and
- backface culling **on** with `glFrontFace(GL_CW)` (set for the inverted-Y
  stereo projection and the procedurally-authored boxes). The baked `.xmd`
  triangles are an **open shell** whose winding is not guaranteed relative to
  that handedness, so faces could be culled and the shell looked single-sided.

Separately, the four hand-local **interaction boxes** (reload/receiver glow,
NanoMED cartridge, scanner-heal glow, grab glow) are authored at *procedural*
hand-local coordinates and were drawn **unconditionally**, so on the native path
they floated near the mesh as "stray procedural blocks" (GH #64 finding).

## Decision
All changes are in `src/hook/vr_hand_renderer.cpp` (+ config/tests/docs):

1. **Two-sided, real lit 3D.** For the native triangle batch only:
   `glDisable(GL_CULL_FACE)` and `glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE)`
   (restored right after). The hand-rolled abs-lambert is **removed**; the
   shipped unit normals now drive the existing smooth `GL_LIGHTING` with
   `GL_FRONT_AND_BACK` colour material, which lights the inner and outer shell.
   A new core GL entry point `glLightModeli` is added to the dispatch table.
2. **Texture tint.** When the GH #61 diffuse is bound (MODULATE), the per-vertex
   colour tint goes to **white** so the real texture shows at full strength
   instead of being darkened by the old `0.55/0.42/0.33` fallback tint; the
   flat skin tone is kept only for the untextured fallback. The per-hand fade
   still scales alpha/colour.
3. **No stray blocks.** `DrawTacticalHand` records the result of
   `DrawNativeHand(side)` once; the interaction boxes are drawn **only when the
   procedural hand is drawn** (`!native`). Re-anchoring the interaction visuals
   to the native mesh frame is future work.
4. **Orientation knob (partial item 1).** `native_hand_pose.h` gains
   `RotateNativeHandPlacement(p, yaw, pitch, roll)` — a hand-local orientation
   correction (degrees; `R = Ry(yaw)·Rx(pitch)·Rz(roll)`, identity by default)
   composed after the basis mapping in `MapNativeHandPoint`/`MapNativeHandNormal`.
   Wired to INI keys `NativeHandYaw/NativeHandPitch/NativeHandRoll`
   (`g_vrConfig`, clamped ±180°). The provisional ADR-0427 axis basis can now be
   corrected on a headset without a code change.

The upper-arm cloth segment (`drawArm`, shoulder→elbow) is **intentionally still
drawn**: the baked arm clusters do not cover the upper arm, so removing it would
open a gap between the HMD-anchored shoulder and the native forearm. Only the
forearm cloth is skipped when native (unchanged, GH #46).

## Consequences
- The native mesh renders as a closed, shaded 3D surface with the real diffuse,
  no missing faces, and no floating blocks.
- The `native_hand_pose_mapping` CTest gains rotation-composition, orthonormality
  and normal-length checks; `verify_reported_fixes.py` gains a #64 invariant
  (two-sided draw, orientation knob, interaction boxes gated on the procedural
  path).
- No new runtime dependency; still fixed-function immediate mode, still fully
  restored via the existing `glPushAttrib`/`glPopAttrib` (the two-sided enable
  and culling toggle are additionally restored inline).

## Not claimed
- **Not validated in-headset.** The orientation correction defaults to 0 (the
  ADR-0427 inferred basis); `NativeHandYaw/Pitch/Roll` and the
  scale/offset/V-flip values must be tuned on hardware. GH #64 acceptance
  ("solid, correctly placed, validated in-headset") remains a headset step.
- The mesh is still drawn **rigidly**; skinning waits on the runtime per-bone
  matrix (GH #60/#58).
- The interaction visuals are **hidden** on the native path, not relocated to the
  native hand frame — no in-hand feedforward cue is available there yet.
