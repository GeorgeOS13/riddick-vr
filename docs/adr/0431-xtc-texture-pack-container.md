# ADR 0431: the `.xtc` texture packs - `MOS DATAFILE2.0` / `VirtualXTC` container

## Status
**Accepted (read-only)** - new read-only reader `tools/mos_xtc.py`; the three
shipped texture packs are decoded structurally and validated. No game file was
modified; no code was deployed; nothing is written to the install.

This continues the `#46` chain: ADR-0420 (native hand mesh) -> ADR-0427 (draw
path) -> **ADR-0431** (the textures those hands must sample), in service of
GH **#61** ("apply the real `.xtc` diffuse to the native hands").

## Context
GH #61 needs Riddick's real skin diffuse for the native hand draw path
(`VRHandRenderer::DrawNativeHand`). The textures are not loose files: no
`.dds/.png/.tga` ships (1 `.tga` in all of `Content`); the images are packed in
`Content/{GUI,Models,Levels}.xtc` (and
`ExtraContent/Offline01/Textures/Offline01.xtc`). `tools/mos_xmd.py` rejected
them ("implausible root node offsets" - the root's `+0x20` offset field is `0`,
not a data-region start). A dedicated reader was required.

## Finding 1 - the pack is the engine's `VirtualXTC` container
A raw printable-string scan of `System/Win32_x86/MSystem.dll` (read-only)
contains the whole texture-container family:

```
C:\...\MSystem.dll  001408B0  CTextureContainer_VirtualXTC2
MSystem.dll  001408E8  CTextureContainer_VirtualXTC
MSystem.dll  00149304  Invalid XTC. (IMAGELIST was not a directory.)
MSystem.dll  00149334  AddFromXTC
MSystem.dll  00149340  Invalid XTC. (No IMAGELIST)
MSystem.dll  0014989C  XTXCompile
MSystem.dll  001499F4  .\Raster\MTextureContainerXTC2.cpp
MSystem.dll  0019A8E2  ?AddFromXTC@CTextureContainer_Plain@@UAEXPAVCDataFile@@@Z
MSystem.dll  0019DB6F  ?Create@CTextureContainer_VirtualXTC2@@UAEXPAVCDataFile@@H@Z
```

So a `.xtc` is a `CDataFile` tree (the same class the `.xmd` uses) whose image
directory chunk is named `IMAGEDIRECTORY5`, and whose pixel payload is the
engine's compressed `XTX` image format. The `.xmd` `KEYS` metadata points at the
same textures by their build path (`XTXPATH = S:\Models\...\Riddick_B03.xtx`,
ADR-0421), i.e. the `.xtx` files are packed into the `.xtc` at build time.

## Finding 2 - container layout (all three game packs identical)
```
root  : 0x30-byte node record at offset 0
        +0x00 char[0x18] "MOS DATAFILE2.0\0"
        +0x18 u32 next_sibling = node-table offset (near EOF)
        +0x1C u32 child        = 0
        +0x20 u32 data_offset  = 0      <- why mos_xmd rejects it
        +0x24 u32 data_size    = 0
        +0x28 u32 count        = 0
        +0x2C u32 flags        = 0
node  : same 0x30-byte record, contiguous from node_start; exactly two:
        TEXTURES         payload = packed texture-data blob, at file 0x30
        IMAGEDIRECTORY5  payload = image directory (last region before nodes)
```
The two payloads tile `[0x30, node_start)` exactly, on all three packs:

| pack | file size | node_start | TEXTURES (off+size) | IMAGEDIRECTORY5 (off+size) |
|---|---|---|---|---|
| `GUI.xtc` | `0x112C9E6` | `0x112C986` | `0x30+0x1127322` | `0x1127352+0x5634` |
| `Models.xtc` | `0x28DEFF6B` | `0x28DEFF0B` | `0x30+0x28DDD11B` | `0x28DDD14B+0x12DC0` |
| `Levels.xtc` | `0x5AE8DCB1` | `0x5AE8DC51` | `0x30+0x5AE40437` | `0x5AE40467+0x4D7EA` |

## Finding 3 - `IMAGEDIRECTORY5` is a flat framed record list
```
u32  marker   = 0xFFFFFFFF
u32  zero     = 0
u32  name_len
char name[name_len]            (ASCII, no NUL)
pad with 0x20 to a 4-byte boundary
u32  desc[14+]                 (>= 48 bytes; 58 for most records)
```
Records are **framed by the `FF FF FF FF 00 00 00 00` marker** and tile the
directory payload exactly: **246** (GUI) / **901** (Models) / **3797** (Levels).
The node `count` field (`+0x28`) reads **248 / 914 / 3811** - i.e. **2-14 more**
than the framed records. Because the framed records tile the payload with no
gap (validated), the framing is authoritative and the count field's exact
meaning is an open question (recorded, not guessed).

## Finding 4 - the descriptor, pinned and cross-checked
`desc` is a little-endian `u32` array. The leading fields are **pinned across all
4944 records** in the three packs:

| idx | meaning | evidence |
|---|---|---|
| 0 | reserved (0) | always 0 in Levels; occasional flags elsewhere |
| 1 | mip level count == `log2(max(w,h)) + 1` | **4944/4944** |
| 2 | `0x300` | constant |
| 3 | `0x5000` colour / `0x0` linear (normal map) | suffix split |
| 4 | base mip byte count, OR `0x10` when block-compressed | see below |
| 5 | width (power of two) | 4944/4944 |
| 6 | height (power of two) | 4944/4944 |
| 7 | unknown (`0x800` typical, `0x20` for tiny) | - |
| 8 | 0 | - |
| 9 | **absolute file offset** of the packed block | strictly increasing, inside `TEXTURES` |
| 10 | `0x102` colour / `0x4300102` linear | suffix split |
| 11 | `0` colour / `0x8000000` linear / `0x2000000` small | - |

`desc[4]` is (base mip bytes) with the `0x10` bit OR-ed in for compressed images:
`Riddick_ArmScar_D` = `0x40010` -> `0x40000` = 262144 = **512x1024 DXT1**;
`riddick_armscar_N` = `0x200000` = 2097152 = **512x1024 RGBA** (uncompressed).
`desc[1]` for that pair is `0xB` = 11 = the 512x1024 mip chain length.

Worked example (`Models.xtc`):
```
Riddick_ArmScar_D   512x1024  mips=11  colour  compressed  base=0x40000  block@0x16CDEABE (+350072 B)
riddick_armscar_N   512x1024  mips=11  linear  raw         base=0x200000 block@0x16688C3E (+2796556 B)
Riddick_Eye_D       128x128   mips=8   colour  compressed
```

## Finding 5 - the packed block framing
A block starts with a **`u32` mip-offset table** (`mip_levels` absolute file
offsets), then one `[0x2C-byte header + data]` record per mip. For
`Riddick_ArmScar_D` (block `0x16CDEABE`):

```
+0x00  EA EA CD 16  -> 0x16CDEAEA  (= block + 0x2C)   mip 0
+0x04  16 EB D1 16  -> 0x16D1EB16                     mip 1   (+0x4002C)
+0x08  42 EB D2 16  -> 0x16D2EB42                     mip 2   (+0x1002C)
 ...
+0x28  02 42 D3 16  -> 0x16D34202                     mip 10
+0x2C  00 03 00 00 00 50 00 00 10 00 04 00 ...        mip 0 header
```
The mip deltas are `0x2C + {0x40000, 0x10000, 0x4000, 0x1000, ...}`, the exact
512x1024 DXT1 mip-chain sizes - decisive that `desc[9]` is a real file offset and
the table is a mip offset list. The **pixel codec itself is not decoded yet**.

## Change
- `tools/mos_xtc.py` (new): read-only `.xtc` reader. Parses the root/2-node
  container, the framed `IMAGEDIRECTORY5` records and the pinned descriptor
  fields; `--list`, `--find NAME`, `--extract NAME --out FILE`, `--json`, and a
  `--check` structural validator. Lazy (seeks) so the 1.5 GB `Levels.xtc` is
  handled without loading it.
- `tools/test_mos_xtc.py` (new): 12 self-contained synthetic-container tests
  (round-trip, find, empty, bad magic, unsupported variant, and validators for
  node gaps, non-power-of-two dims, mip mismatch, out-of-blob offsets,
  non-monotonic offsets, `desc[2]`, non-tiling directory). No game install.
- `.github/workflows/vr-worker.yml`: run `test_mos_xtc.py`.
- `docs/rea/mos-xmd-format.md`: new `.xtc` section.

## Evidence (commands)
- `python tools/mos_xtc.py --check .../GUI.xtc|Models.xtc|Levels.xtc`
  -> `ok` 246 / 901 / 3797 images.
- `python tools/test_mos_xtc.py` -> PASS (12 tests).
- `python tools/mos_xtc.py .../Models.xtc --find Riddick_ArmScar_D` ->
  `512x1024 mips=11 color=True compressed=True base_mip=262144B
  block@0x16CDEABE+350072B`.
- Raw string scan of `MSystem.dll` (Finding 1); `Levels.xtc` (1.5 GB) parsed
  without loading it.

## Not claimed / follow-ups
- The pixel codec (the block data looks block-compressed / `XTX`; `desc[4] & 0x10`
  marks compression, but DXT variant, palette, and any swizzle are **not**
  decoded). Next: decompile `CTextureContainer_VirtualXTC2` / `XTXCompile` in
  `MSystem.dll` (needs a `rea search MSystem.dll "VirtualXTC2" --kind procedures`
  run) and decode `desc[7]`/`desc[10]`/`desc[11]` bits.
- The node `count` (`+0x28`) vs framed-record count discrepancy (Finding 3).
- `Offline01.xtc` uses a different nested variant
  (`SOURCE`/`IMAGELIST`/`IMAGEDIRECTORY4`/`IMAGE`/`NAME`/`PROPERTIES`/`MIPMAP`);
  out of scope. `tools/mos_xtc.py` reports it as an unsupported variant.

## Files
- `tools/mos_xtc.py`, `tools/test_mos_xtc.py`,
  `.github/workflows/vr-worker.yml`, `docs/rea/mos-xmd-format.md`.
