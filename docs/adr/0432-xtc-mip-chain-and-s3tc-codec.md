# ADR 0432: the `.xtc` packed-block mip chain and its S3TC codec

## Status
**Accepted (read-only)** — extends `tools/mos_xtc.py` with the per-image mip
chain and a standard S3TC decoder (`DXT1`/`DXT3`/`DXT5`). No game file was
modified; nothing is written to the install. The engine reader behind this was
read statically (Ghidra 12.1.4 via `rea`) with no execution.

Continues ADR-0431 (the `.xtc` container) in service of GH **#61** ("apply the
real `.xtc` diffuse to the native hands").

## Context
ADR-0431 recovered the container (`TEXTURES` + `IMAGEDIRECTORY5`) and the
descriptor, but left the packed block's *pixel codec* open: the block looked
block-compressed, `desc[4] & 0x10` flagged compression, and the base-mip byte
counts exactly matched DXT1 (512x1024 -> `0x40000`) — but the per-mip framing
and codec were not decoded. GH #61 needs the real diffuse pixel data, so the
codec had to be pinned.

## Findings

### 1. The packed block is a mip chain (table + per-mip header + data)
For every image (block at `desc[9]`):

```
u32 mip_offset[mip_count]        absolute file offsets of each mip's header
per mip:  header (0x2C compressed | 0x1C raw) then the mip pixel data
```

`mip_count` is read back from the table itself —
`mip_count = (mip_offset[0] - block_offset) / 4` — because the table is exactly
`4 * mip_count` bytes. The chain invariant
`mip_offset[i] + header_len + mip_bytes == mip_offset[i+1]` holds on
**887/901** `Models.xtc`, **244/246** `GUI.xtc` and **3787/3797** `Levels.xtc`
images (the remainder are a reduced-mip-chain variant, see *Not claimed*).

Per-mip header fields (little-endian `u32`):

| off | compressed (0x2C) | raw / linear (0x1C) |
|---|---|---|
| +0x00 | `0x300` | `0x300` |
| +0x04 | colours (`0x5000`) or `0` | `0` |
| +0x08 | `mip_bytes \| 0x10` | `mip_bytes` |
| +0x0C | width | width |
| +0x10 | height | height |
| +0x14 | `0x800` | `0x800` |
| +0x18 | `0` | `0` |
| +0x1C | **codec** (`0`/`2`/`4`) | — (pixel data follows) |
| +0x20 | `mip_bytes` | — |
| +0x24 | `0` | — |
| +0x28 | `0x10` | — |

A mip whose `mip_bytes == 0` is an **empty** mip: a header only, no pixel data
(seen on the smallest levels of some `*_PS2` textures).

### 2. The codec is standard S3TC; the enum is `0=DXT1, 2=DXT3, 4=DXT5`
The header `+0x1C` codec field correlates **1:1** with the stored byte count:
`0` -> `w*h/2` (DXT1/BC1), `2` and `4` -> `w*h` (DXT3/BC2 and DXT5/BC3). Across
`Models.xtc`: `455` images codec `0`, `128` codec `4`; `GUI.xtc`: `43`/`104`;
`Levels.xtc` additionally uses codec `2` (`290` mips). Decoding each as the
standard S3TC variant reproduces the shipped art, **verified visually**:

| image | codec | result |
|---|---|---|
| `Riddick_ArmScar_D` (512x1024, `Models.xtc`) | `0` DXT1 | Riddick's arm skin + the stitched "arm scar" |
| `Riddick_B03_D` (512x1024, `Models.xtc`) | `0` DXT1 | body skin + tank top + scar |
| `Pa2_inmate_H04_D` (512x512, `Models.xtc`) | `4` DXT5 | a coherent inmate head UV map |
| `SKYMOUNT_0` (512x512, `Levels.xtc`) | `2` DXT3 | a snowy-mountain skybox silhouette |

The block order is the usual S3TC order (alpha data first, then the colour
block): decoding `Pa2_inmate_H04_D` with the two 8-byte halves swapped yields
garbage; the standard order yields the head. `Riddick_ArmScar_D`'s base mip
begins `11 C5 F0 C4 0A 2B 08 A0` — a plain `RGB565 + 2-bit-index` BC1 block.

### 3. The engine reader (static, read-only)
`rea search MSystem.dll "XTC" --kind procedures` names the family, and
`rea decompile` gives:

- `CTextureContainer_VirtualXTC2::ReadTexture` (`0x1009b280`) and
  `CTextureContainer_VirtualXTC2::GetTextureMipMap` (`0x1009b760`) — the mip
  reader. `GetTextureMipMap` validates the requested level against a per-image
  `min_mip`/`max_mip` byte (`descriptor+0x19`) and returns a `CImage`
  (`.\\Raster\\MTextureContainerXTC2.cpp`, `"Invalid mipmap requested: %d/%d/%d,
  texture %s"`).
- `CImage::ReadHeader` (called from `ReadTexture`) and the `CImagePalette`
  path handle the optional palette and the header decode.

The tool mirrors the *file* layout these functions consume; it does not run
them.

## Change
- `tools/mos_xtc.py`:
  - `Xtc.read_mips(image)` / `try_read_mips(image)` -> `XtcMip` list
    (`level/width/height/size/codec/compressed/data_offset/header`), with the
    chain invariant enforced.
  - `bc1_decode` / `bc2_decode` / `bc3_decode` (+ `_rgb565`, `_bc1_base`) and
    `Xtc.decode(image, level)` -> `(w, h, rgba)`; `write_png` (stdlib-only).
  - CLI: `--mips NAME`, `--decode NAME --out FILE.png [--mip N]`; `summary()`
    gains `mip_chain_ok_images` / `mip_chain_bad_images` /
    `mip_codec_histogram`; the block report prints the mip-chain counts.
- `tools/test_mos_xtc.py`: `+6` tests (BC1/BC2/BC3 decode on known blocks, an
  empty mip, a raw 32-bit image, a PNG write, and a chain-break report) ->
  **18** total. `build_xtc` now emits real packed mip blocks.
- `docs/rea/mos-xmd-format.md`: `.xtc` item 4 rewritten (mip chain + codec).

## Evidence (commands)
- `python tools/mos_xtc.py <pack> --mips Riddick_ArmScar_D` ->
  `11 parsed`, levels `512x1024 DXT1 262144B` .. `1x1 DXT1 8B`.
- `python tools/mos_xtc.py <Models.xtc> --decode Riddick_ArmScar_D --out
  armscar.png` -> a correct skin/scar texture (viewed);
  `--decode Pa2_inmate_H04_D` (DXT5 head, viewed);
  `python tools/mos_xtc.py <Levels.xtc> --decode SKYMOUNT_0` (DXT3 sky, viewed).
- Chain/mip stats: `Models.xtc` ok 887 / bad 14, codecs
  `{DXT1:4285, raw32:3113, DXT5:1179}`; `GUI.xtc` ok 244 / bad 2;
  `Levels.xtc` ok 3787 / bad 10, `{DXT1:18833, raw32:13037, DXT5:2600,
  DXT3:290}`.
- `python tools/mos_xtc.py --check GUI/Models/Levels.xtc` -> `ok`
  (246 / 901 / 3797); `python tools/test_mos_xtc.py` -> PASS (18).
- RE (read-only): `rea search MSystem.dll "XTC" --kind procedures`;
  `rea decompile MSystem.dll 0x1009b280` (`ReadTexture`), `0x1009b760`
  (`GetTextureMipMap`), `0x1009c690` (`ReadImageDirectory`).

## Not claimed / follow-ups
- The pixel format of the **raw / linear** images (base mip `w*h*4` bytes, e.g.
  normal maps `riddick_armscar_N`) is **not** decoded: the channel order is not
  verified, so `--decode` emits the file bytes in order. This is not needed for
  the #61 diffuse (which is DXT1) but is needed for lit/bumped shading.
- Why a handful of images ship a **reduced / non-conforming** mip chain
  (`mip_count < desc[1]`, e.g. `SCam_D` 9 of 10; 14+2+10 images) is open; the
  reader reports them rather than guessing.
- `desc[0]`/`desc[7]`/`desc[10]`/`desc[11]` bit meanings, and the
  `descriptor+0x19` `min_mip`/`max_mip` byte's exact file offset, are only
  partially mapped.
- `Offline01.xtc`'s nested variant (`SOURCE`/`IMAGELIST`/`IMAGEDIRECTORY4`/...)
  is still unsupported.
- **Next for #61:** feed `bc1_decode` of `Riddick_ArmScar_D` into
  `VRHandRenderer::DrawNativeHand` (or bake it into a small in-tree texture
  asset, mirroring the ADR-0420 mesh bake).

  **Resolved (ADR-0433, GH #61):** the in-tree texture asset was chosen —
  `tools/xtc_bake.py` bakes the verbatim DXT1 blocks (mip 1, 256x512) into
  `src/hook/native_hand_texture_data.h`; `src/hook/bc1.h` decodes them and
  `DrawNativeHand` binds the result. Validation on a headset (UV V-flip) is
  still open.

## Files
- `tools/mos_xtc.py`, `tools/test_mos_xtc.py`, `docs/rea/mos-xmd-format.md`,
  `docs/adr/0432-xtc-mip-chain-and-s3tc-codec.md`.
