# ADR 0433: the native-hand diffuse texture asset (`.xtc` DXT1 bake)

## Status
**Accepted (read-only on the game)** — adds an in-tree, compile-in texture asset
and decodes it in the hook. No game file was modified; the game install was only
read. Continues ADR-0431/0432 (the `.xtc` container and codec) in service of GH
**#61** ("apply the real `.xtc` diffuse to the native hands").

## Context
ADR-0432 pinned the `.xtc` packed block (a mip chain of `DXT1`/`DXT3`/`DXT5`
S3TC surfaces) and decoded `Riddick_ArmScar_D` to a correct skin/scar image with
`tools/mos_xtc.py`. The native hand arm clusters (`Riddick_LeftArm` /
`Riddick_RightArm`, ADR-0420/0424/0427) carry per-vertex UVs that span the full
`512x1024` atlas, so the real diffuse can now be bound — but the hook must not
open the **685 MB** `Content/Models.xtc` at runtime, and re-encoding the texture
would lose the shipped data.

## Decision
Bake the shipped DXT1 blocks **verbatim** into a generated C++ header and decode
them in the hook:

- `tools/xtc_bake.py` reads one image's mip out of a `.xtc` (via
  `tools/mos_xtc.py`), requires a **compressed DXT1** mip, and emits
  `src/hook/native_hand_texture_data.h`: the raw BC1 blocks, the image metadata,
  a handful of **decoded reference pixels** and a **checksum** of the whole
  decoded RGBA surface.
- `src/hook/bc1.h` (`Bc1DecodeBlock` / `Bc1DecodeSurface`) turns the blocks into
  top-down RGBA8; the maths mirrors `tools/mos_xtc.py` (`_rgb565` /
  `_bc1_base` / `bc1_decode`) exactly — integer 5/6/5 expansion and integer
  palette interpolation.
- `VRHandRenderer::DrawNativeHand` lazily creates one `GL_TEXTURE_2D`, uploads
  uncompressed RGBA8 (no GL extension required), enables it for the native hand
  triangles only, and emits `glTexCoord2f` from the baked UVs with
  `GL_MODULATE` so the existing lit colour (fade + two-sided lambert) still
  applies. When the texture is unavailable the flat skin tone remains.

**Canonical bake:** `Riddick_ArmScar_D`, **mip 1** (`256x512`, the engine's own
mip — no re-encode), 65536 block bytes, from `Content/Models.xtc`. Mip 0
(`512x1024`, 262144 bytes) is available with `--mip 0` for full resolution.

## Why the compressed blocks (not RGBA) are stored
- Verbatim: the shipped S3TC data is kept, no decode/re-encode loss.
- Small: the 256x512 DXT1 payload is 64 KB (a same-size RGBA8 bake would be
  512 KB); the generated header is 411 KB, in line with the existing
  `native_hand_mesh_data.h` (205 KB).
- Cross-checkable: the independent Python decoder produces the reference pixels
  and checksum, so the C++ decoder is proven, not asserted.

## Evidence
- `python tools/xtc_bake.py --cpp src/hook/native_hand_texture_data.h
  --manifest docs/rea/reference/native_hand_texture.json` ->
  `1 texture(s), 65536 block bytes` (mip1 `256x512 DXT1`,
  `sha256 21c57eca…021e`, `rgba_checksum 0x05308AE2`).
- `python tools/xtc_bake.py --cpp src/hook/native_hand_texture_data.h --verify`
  -> `OK: … matches a fresh bake`.
- `native_hand_texture_asset` (`tools/build-all.ps1` + `ctest build/x86|x64`)
  -> PASS: hand-computed BC1 blocks, edge clipping without overrun, and the
  full-surface **checksum + all 8 reference pixels** decoded by `bc1.h` equal the
  Python-decoded reference.
- `python tools/test_xtc_bake.py` -> PASS (round-trip, manifest, `--verify`
  stale detection, non-DXT1 / unknown-image / out-of-range-mip rejection).
- Visual (read-only, for the record): `mos_xtc.py --decode Riddick_ArmScar_D`
  reproduces the arm skin + stitched scar (ADR-0432).

## Not claimed
- **Not validated on a headset.** The UV orientation (`NativeHandTextureVFlip`,
  default 0) and the mesh placement (ADR-0427) are provisional; the flat skin
  tone and the procedural overlay stay as fallbacks.
- The texture is bound with the **fixed-function** pipeline and `GL_MODULATE`;
  there is no normal-map/specular (`riddick_armscar_N` / `_S`) and no
  per-material shading.
- Only **DXT1** is baked; DXT3/DXT5 and the raw/linear normal maps are not
  (the raw format's channel order is still undecoded, ADR-0432).
- The asset is the **body arm** surface (`RIDDICK_ARMSCAR_D`), matching the
  `Riddick_B05` arm clusters, not a dedicated first-person viewmodel.

## Files
- `tools/xtc_bake.py`, `tools/test_xtc_bake.py`,
  `src/hook/bc1.h`, `src/hook/native_hand_texture.h`,
  `src/hook/native_hand_texture_data.h` (generated),
  `src/hook/vr_hand_renderer.{h,cpp}`, `src/hook/vr_config.{h,cpp}`,
  `tests/native_hand_texture_tests.cpp`, `CMakeLists.txt`,
  `docs/rea/reference/native_hand_texture.json`,
  `docs/rea/mos-xmd-format.md`, `.github/workflows/vr-worker.yml`.
