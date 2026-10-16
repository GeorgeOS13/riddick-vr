# ADR 0439: Per-Draw Engine Viewmodel Suppression (GH #63 approach 2)

## Status
Proposed — infrastructure landed, **off by default, not validated in-headset**.
The pass signature is a hypothesis (see "Provisional signature").

## Context
1. GH #63 ("duplicate hands") requires hiding the engine's *own* first-person
   arms/hands while the VR hands render.
2. **Approach 1 is out** (ADR-0438): the `noviewmodel` bit is bit 1 of
   `CRPG_Object_Item + 0x14c`, but the engine **sets and clears it itself** every
   frame (`FUN_100e6aa0`) and an exhaustive raw-byte scan of every shipped x86
   module finds **no direct `[+0x14c] & 2` reader**. A forced write has no
   consumer and would be fought by the engine's own state machine.
3. GH #66 filed approach 2: intercept the draw calls and skip the ones that
   belong to the first-person viewmodel pass.
4. **Render-path evidence** (ADR-0439 companion, `docs/rea/reference/RndrGL_GL_Path.txt`):
   - `RndrGL.dll` is the **only** shipped module that imports OpenGL, so an IAT
     hook there sees every engine draw.
   - Its 3D mesh path is **`glDrawElements`** (indexed vertex arrays); **no**
     shipped module imports `glDrawArrays`.
   - `glCallList` is also imported (list *building* entry points are absent — an
     open question; the call is intercepted opportunistically).
   - `glDepthRange` is **not** imported (so the depth-bias trick is not used and
     depth range is not a usable signal); `glMultMatrixf`/`glRotatef` are absent
     — transforms come from `glLoadMatrixf`/`glPushMatrix`/`glTranslatef`, and
     `glClear`/`glFrustum` are already intercepted.

## Decision
1. **Intercept the draw entry points that matter, and only in `RndrGL.dll`.**
   `InstallRndrGLHooks` (`src/hook/dllmain.cpp`) IAT-hooks `OPENGL32.dll!
   glDrawElements` and `OPENGL32.dll!glCallList` using the existing `IATHook`
   (the same mechanism already in use for `GDI32!SwapBuffers`). Each hook calls
   `OpenGLProxy::ShouldSuppressDrawCall()` and, when it is false, forwards to the
   **saved original** — which is the proxy's own forwarder, so passing through is
   byte-identical to today. No export/`.def`/forwarder change is needed.
2. **The decision is a pure, dependency-free state machine**
   (`src/hook/viewmodel_suppressor.h`), fed by GL signals the hook **already**
   intercepts:
   - a **full colour clear** (`glClear` with `GL_COLOR_BUFFER_BIT`) is the frame
     boundary — it closes the window and re-bases the per-frame counters;
   - a **depth-only clear** (`GL_DEPTH_BUFFER_BIT`, no colour bit) opens the
     window (when that opener is selected);
   - a **projection load** (`glLoadMatrixf`/`glFrustum`/`glOrtho` while the
     matrix mode is `GL_PROJECTION`) closes the window (the world resumes);
   - the buffer **swap** closes the window and latches the frame tallies.
3. **Conservative and reversible.** The feature is **off by default**
   (`SuppressEngineViewmodel=0`); `ViewmodelDepthClearWindow` selects the opener
   (1 = depth-only clear) or disables the window entirely (0). Only the matched
   in-window draws are skipped; the procedural/native VR hands remain the visible
   substitute (`VRHandRenderer` is unchanged). The INI is re-read on every
   `glClear`, so a config edit takes effect live.
4. **Instrumented for calibration.** Every 300 swaps the hook logs
   `Viewmodel suppression [#N]: last frame draws=.. suppressed=.. total=..`, so a
   single headset run shows whether the window matches the first-person pass and
   how many draws it contains.

## Provisional signature (must be confirmed on a headset)
The "mid-frame depth-only clear opens the first-person pass" boundary is the
classic approach in this era (draw the world, clear depth, draw the viewmodel so
it never intersects walls) and is consistent with RndrGL importing `glClear`, but
it is **not yet observed at runtime**. It is deliberately off by default and the
counters exist precisely to confirm or falsify it. If it is wrong, the alternatives
already reachable without new hooks are: the projection boundary alone, the
camera-space modelview translation magnitude, or an explicit draw index within
the frame — all expressible as new `ViewmodelWindowOpener` values in the same
tested header.

## Consequences
- The duplicate-hands path no longer depends on the falsified `noviewmodel` bit.
- With the feature off there is **zero** behaviour change: `ShouldSuppressDraw()`
  returns `false` and each hook forwards to the saved original.
- Unit-tested headlessly (`viewmodel_suppressor_window`,
  `tests/viewmodel_suppressor_tests.cpp`) — no game launch, no synthesized input.
- A source invariant (#66) is enforced in `tools/verify_reported_fixes.py`
  (off-by-default, RndrGL interception, saved-original forwarding, test present).
- Remaining risk is contained to `glDrawElements`/`glCallList` inside the window;
  world geometry and weapons are untouched when the flag is off or the window
  never opens.

## Open questions / follow-ups
- Confirm in-headset that the depth-only-clear window matches the first-person
  pass, and that enabling it hides the arms without dropping world geometry.
- Resolve the `glCallList` list-ID origin (`docs/rea/reference/RndrGL_GL_Path.txt`
  §2); add a begin/end pair gate only if the arms turn out to use immediate mode.
- Add a camera-space / projection-fingerprint opener if the depth-clear boundary
  proves unreliable.
