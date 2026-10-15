# ADR 0417: `.xmd` `BONEDEFORM` skinning layout recovered from MXR.dll

## Status
**Accepted** — the `BONEDEFORM` payload layout (per-vertex bone influences +
bone matrix map) is decoded from the shipped loader (`MXR.dll`) and re-validated
across all 631 shipped models (3103 skinned clusters). Read-only analysis; no
game file was modified and no code is deployed.

This is the skinning step of GH #55 (advancing GH #46), following the container
layout of ADR-0414, the position/triangle geometry of ADR-0415, and the UVs of
ADR-0416.

## Context
ADR-0415/0416 decoded positions, normals, indices, and UVs. A native first-person
hand/arm path additionally needs to know how each vertex is bound to the
skeleton so the mesh can be posed by the tracked hand/arm transform. In
`CTriangleMeshCore::Read`, every cluster is written as `VERTEXFRAMES` →
`TVERTEXFRAMES` → skinning → `TRIANGLES`. When the **model version is > 0x100**
(shipped models report `0x102`) the skinning is a single embedded `BONEDEFORM`
chunk; for older models it is three separate top-level
`BONEVERTEXINFO`/`BONEINFLUENCE`/`BONEMATRIXMAP` chunks.

## Evidence (2026-10-15, read-only; Ghidra 12.1.4 via `rea`)
Target: `System/Win32_x86/MXR.dll` (sha256
`06424f55f5a0f85eb09b5fbd69491c7f6a88de808c26c513e8dac180f3c2052d`,
image base `0x10000000`).

- `FUN_10080f50` (`CTriangleMeshCore::Read`) selects the embedded form
  (`CDataFile::GetNext("BONEDEFORM")`) when `model_version > 0x100`, saving the
  stream position and calling **`FUN_1007d260`**. For older models it reads the
  three separate chunks and normalizes them in place.
- `FUN_1007d260` decompiles as the embedded `BONEDEFORM` reader:
  1. `u32 n` → `CListVectorCore::Core_SetLen` of the per-vertex array, then a
     raw `CCFile::Read` of `n * 4` bytes into it. Each record is indexed at a
     **4-byte** stride and the loop reads `*(ushort*)(record+2)` (a start index)
     and `*(char*)(record+1)` (an influence count); byte 0 is unread.
  2. `u32 m` → sizes the influence array, then
     `FUN_1007c150(stream, 0x200, dst, m)`.
  3. If `model_version > 0x101`, a trailing `u32` is read (the matrix-map id),
     and when non-empty a `u32` palette count followed by that many bytes.
  4. The reader then **repairs** the data: it walks the vertex records, takes the
     influence subrange `[start, start+count)`, and rewrites the *last* weight to
     `1 - sum(previous)`, so `influence_start` is the running prefix sum of
     `influence_count`.
- `FUN_1007c150` (version `0x200` branch, the one `BONEDEFORM` uses) reads the
  influence pool in blocks of up to 128 entries. The on-disk entry is **2 bytes**:
  `u8 bone`, `u8 weight_q`; the decoded weight is `weight_q * 0.003921569`
  (= `1/255`). (The `< 0x200` branch uses a different, wider record and is not
  exercised by any shipped asset.)
- Empirically the payload tiles exactly as
  `4 + 4n + 4 + 2m + 8 + b` bytes, with `n` equal to the paired `VERTEXFRAMES`
  vertex count, `m` equal to the sum of the vertex `influence_count`s, and `b`
  the trailing matrix-map palette size — an independent arithmetic check on the
  recovered field order/width.

## Recovered layout
```
BONEDEFORM (embedded form; selected when model_version > 0x100):
    u32  vertex_count          n
    n * { u8 flags;            (0 in every shipped vertex)
          u8 influence_count;  number of bones affecting this vertex
          u16 influence_start }running prefix sum of influence_count
    u32  influence_count       m
    m * { u8 bone;             skeleton node index
          u8 weight_q }        weight = weight_q / 255
    u32  matrix_map_id         (s32; 0xFFFFFFFF = none)
    u32  matrix_bone_count     b
    b * u8                     bone matrix-map palette indices

payload size = 4 + 4n + 4 + 2m + 8 + b bytes
```

The trailing matrix-map block is present for every shipped cluster (the
model version `0x102` selects it). A cluster with no skinning omits the
`BONEDEFORM` node entirely; no shipped model uses the separate
`BONEVERTEXINFO`/`BONEINFLUENCE`/`BONEMATRIXMAP` chunks.

## Verification
- `python tools/mos_xmd.py --inventory <game>\Content\Models --check`:
  **631/631 files `ok`**, 0 failures, with the new skinning cross-checks enabled
  (skin/vertex count agreement, prefix-sum starts, exact payload tiling, bone
  index < skeleton node count).
- Aggregate decode over all shipped models (`tools/mos_xmd.py` API):
  **4662 clusters**, **3103 skinned** (1559 unskinned/rigid) with **1,315,626**
  decoded influences; **max 3 influences per vertex**. Every skinned cluster
  decodes with **0** bad clusters, **0** out-of-range bone indices, and **0**
  tiling failures; all 3103 carry the matrix-map tail.
- Per-vertex influence weights sum to exactly `1.0` across representative models
  (`Riddick_H01`, `Riddick_B03`, `Riddick_LGuard`, `Riddick_H_Civ`): **0**
  violations. Example `Riddick_H01` cluster 0 (`985` verts, `1564` influences):
  `v0 = {3:0.008, 1:0.102, 0:0.890}`; `max_bone = 17` (< `70` skeleton nodes).
- `tools/test_mos_xmd.py` extended with headless synthetic-container tests:
  skin round-trip + weight scale + `influences_of`, matrix-map decode,
  prefix-sum mismatch, skin/vertex count mismatch, out-of-range bone index,
  bad matrix-map tiling, and the no-`BONEDEFORM` case.

## Consequences
- The real per-vertex skinning of every `.xmd` mesh is now extractable read-only
  alongside positions, normals, indices, and UVs. The native hand/arm path of
  GH #46 / Option A of ADR-0413 can now pose the game's actual mesh with the
  game's actual bone weights instead of a procedural overlay.
- Bone **names** still need decoding from the `NODES` chunk (skeleton); the
  recovered influence indices already range-check against `NODES.count`.
- `BONEDEFORM.flags` (byte 0) is decoded and preserved but uninterpreted (it is
  `0` for all 834,041 shipped vertex records).
- The legacy (model version ≤ `0x100`) split-chunk skinning path is recognised
  but not decoded; it does not occur in the shipped content.

## Follow-ups
- Decode the `NODES` skeleton (names + hierarchy) and the `NODEINDICES` map so
  bone indices can be resolved to named joints (e.g. hand/forearm bones).
- Extract the Riddick hand/forearm surfaces (`RIDDICK_ARMSCAR_*`, `Pa2_Hand_01`)
  with geometry + UVs + skin weights and add a native mesh path behind a flag,
  keeping `VRHandRenderer` as the fallback until a path is tested in-headset.
