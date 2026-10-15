# ADR 0415: `.xmd` vertex-frame geometry + triangle layout recovered from MXR.dll

## Status
**Accepted** — the `VERTEXFRAMES` and `TRIANGLES` payload layouts are decoded
from the shipped loader and re-validated across all 631 shipped models
(4662 cluster pairs). Read-only analysis; no game file was modified and no code
is deployed.

This is the geometry step of GH #55 (advancing GH #46), following the container
layout of ADR-0414 and the investigation of ADR-0413.

## Context
ADR-0414 pinned the `MOS DATAFILE2.0` container (node tree + payload tiling), so
the remaining blocker for extracting Riddick's real hand/arm mesh was the byte
layout of the per-chunk geometry payloads. The container strings
(`VERTEXFRAMES`, `TVERTEXFRAMES`, `BONEDEFORM`, `TRIANGLES`, `NODES`, ...) and
their readers live in **`MXR.dll`**, not `MCCDyn.dll` (which only implements the
generic `CDataFile` container).

## Evidence (2026-10-15, read-only; Ghidra 12.1.4 via `rea`)
Target: `System/Win32_x86/MXR.dll` (sha256
`06424f55f5a0f85eb09b5fbd69491c7f6a88de808c26c513e8dac180f3c2052d`,
PDB `Dll_XR.pdb`; image base `0x10000000`).

- Chunk-name literals are in `.rdata` at `0x10149c70`+ (`PRIMITIVES`,
  `TRIANGLES`, `BONEDEFORM`, `TVERTEXFRAMES`, `VERTEXFRAMES`). `rea trace
  VERTEXFRAMES` resolves exactly two referencing procedures:
  `FUN_1007d7b0` and `FUN_10080f50`.
- `FUN_10080f50` decompiles as **`CTriangleMeshCore::Read`**. It walks the model
  with `CDataFile::GetNext(<chunk>)` in this order: `MISCELLANEOUS` →
  `VERTEXFRAMES` → `TVERTEXFRAMES` → (`BONEVERTEXINFO`/`BONEINFLUENCE`/
  `BONEMATRIXMAP` when model version < `0x101`, else `BONEDEFORM`) →
  `TRIANGLES` → `PRIMITIVES` → (`EDGES`/`EDGETRIS`/`TRIEDGES`).
- `FUN_10006620` = read one **`u32` (4 bytes LE)**; `FUN_10019390` = read **3
  floats**.
- `FUN_1007fed0` decompiles as **`CVertexFrame::Read`**: the first word is the
  *vertex-frame version* and it switches on `0x200`/`0x201`/`0x202`/`0x203`/
  `0x204` (else `throw "Unsupported vertex-frame version %.4x"`).
- For **`0x204`** (the version every shipped model uses) `CVertexFrame::Read`
  reads a header, records the stream position, then calls
  `FUN_1007c6c0` (positions) and `FUN_1007ca20` (normals):
  - `FUN_1007c6c0`: `seek(pos); n = u32; u16[n][3]` → dequantized as
    `min[c] + q * (max[c]-min[c]) / 65535` (the constant `1.5259022e-05` is
    exactly `1/65535`).
  - `FUN_1007ca20`: `seek(pos); n = u32; RelSeek(n*6)` (skip the `u16` positions)
    then `s8[n][3]` → `c/127` (constant `0.007874016` = `1/127`) and
    per-vertex re-normalized to unit length (zero-length stays zero).
- `TRIANGLES` (model version ≤ `0x100`) is read by
  `CCFile::ReadLE(stream, dst, n*3)` after allocating `n` elements: a leading
  `u32 triangle_count` then `triangle_count` **`u16` triplets**.

## Recovered layout
```
model version    : read from MISCELLANEOUS when >1 mesh; shipped models = 0x102.
                   (>0x100 selects the "extended" chunk set.)

VERTEXFRAMES      : (only when the model version is > 0x100) a leading u32
                    frame-count word is read once and discarded, then the
                    per-frame payloads follow. One frame (version 0x204):
    u32  version = 0x204
    u32  header20            (unread by the decode paths)
    f32  bounds_min[3]       quantization lower bound
    f32  bounds_max[3]       quantization upper bound
    f32  aux[3]              third header vector (unread)
    u32  header30            (unread)
    u32  vertex_count  n
    u16  positions[n][3]     x,y,z in 0..65535
    s8   normals[n][3]       -127..127

    frame size = 52 + 9*n   bytes (chunk = 4 + frame when the lead word exists).

TRIANGLES         : u32 triangle_count, then triangle_count * (u16,u16,u16).
```

### Verification
- `python tools/mos_xmd.py --inventory <game>\Content\Models --check`:
  **631/631 files `ok`** with the new geometry cross-check enabled.
- Aggregate decode over all shipped models (`tools/mos_xmd.py` API):
  **4662 cluster pairs**, of which **4662/4662** have every triangle index
  `< n` **and** `max(index) == n-1` exactly (zero unsupported versions, zero
  corrupt frames). Totals: **1,242,285** decoded vertices, **1,860,184**
  triangles.
- The `52 + 9*n` frame size tiles every `VERTEXFRAMES` chunk exactly (no
  remainder) for all 4662 clusters — an independent arithmetic check that the
  field order and width are right.
- `tools/test_mos_xmd.py` extended with synthetic-container tests (decode
  round-trip, out-of-range triangle index, triangle-size mismatch) that run
  headlessly without the game install.

## Consequences
- The real geometry (positions + normals + triangles) of every `.xmd` mesh is now
  extractable read-only; this unblocks the native hand/arm mesh path of GH #46 /
  Option A of ADR-0413.
- `header20`, `header30`, `aux[3]`, and the leading frame-count word are decoded
  and preserved but not yet interpreted; they do not affect positions/indices.
- Versions `0x200`/`0x201` store unquantized float vertex records and are **not**
  decodable by this parser; they do not occur in the shipped content.

## Follow-ups
- Decode `TVERTEXFRAMES` (UVs; its `TVERTEXFRAMES` node carries flags `0x201`)
  and `BONEDEFORM`/`BONEINFLUENCE`/`BONEMATRIXMAP` (skinning) so a decoded mesh
  can be skin-weighted, then extract the Riddick hand/forearm surfaces
  (`RIDDICK_ARMSCAR_*`, `Pa2_Hand_01`).
  - **Done (ADR-0416):** `TVERTEXFRAMES` decoded (version 0x200, size `20+4n`).
  - Remaining: skinning (`BONEDEFORM`/`BONEINFLUENCE`/`BONEMATRIXMAP`).
- Render the extracted mesh in `VRHandRenderer`, keeping the procedural fallback
  until a native path is tested in-headset.
