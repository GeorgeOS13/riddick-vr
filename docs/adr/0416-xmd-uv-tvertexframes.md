# ADR 0416: `.xmd` `TVERTEXFRAMES` UV layout recovered from MXR.dll

## Status
**Accepted** — the `TVERTEXFRAMES` payload layout is decoded from the shipped
loader (`MXR.dll`) and re-validated across all 631 shipped models (4662
clusters). Read-only analysis; no game file was modified and no code is
deployed.

This is the texturing step of GH #55 (advancing GH #46), following the container
layout of ADR-0414 and the position/triangle geometry of ADR-0415.

## Context
ADR-0415 decoded positions, normals, and triangle indices, but a native
first-person hand/arm mesh cannot be textured without UVs. `CTriangleMeshCore::Read`
reads a `TVERTEXFRAMES` chunk per cluster (right after `VERTEXFRAMES`); its
per-frame reader is the procedure called with the model's tvertex version.

## Evidence (2026-10-15, read-only; Ghidra 12.1.4 via `rea`)
Target: `System/Win32_x86/MXR.dll` (sha256
`06424f55f5a0f85eb09b5fbd69491c7f6a88de808c26c513e8dac180f3c2052d`,
image base `0x10000000`).

- `FUN_10080f50` (`CTriangleMeshCore::Read`) reads `TVERTEXFRAMES` via
  `CDataFile::GetNext`, sizes the UV list (`FUN_1007fce0`, the TVERTEX twin of
  `FUN_1007fba0`), then loops calling **`FUN_100807d0(entry, stream, version, defer)`**
  once per cluster.
- `FUN_100807d0` switches on `version`:
  - `version == 0x200` → the shipped path (see layout below);
  - `version == 0x201` → a deferred/seeking form (stores the stream position and,
    when the `defer` flag is set, `RelSeek(count*4 + 0x10)`); not exercised here;
  - `version == 0` → raw `u32[n][2]`;
  - else `throw "Unsupported version %.4x"`.
- **Version 0x200 branch** (decompiled): `n = u32`; then four `f32`
  (`min_u`, `min_v`, `max_u`, `max_v`); then it reads the `u16[n][2]` payload in
  blocks of up to 128 and dequantizes each component as
  `min + q * (max - min) * 1.5259022e-05`
  (`1.5259022e-05` is exactly `1/65535`, the same constant ADR-0415 recovered for
  positions). Stored as two `f32` per vertex.
- Empirically every non-empty shipped `TVERTEXFRAMES` chunk tiles exactly as
  `20 + 4*n` with `n` equal to the paired `VERTEXFRAMES` vertex count, which
  pins the field order/width independently of the decompiler.

## Recovered layout
```
TVERTEXFRAMES (version 0x200, selected by every shipped asset):
    u32  uv_count  n
    f32  min_u
    f32  min_v
    f32  max_u
    f32  max_v
    u16  uv[n][2]        u,v in 0..65535

    u = min_u + q_u * (max_u - min_u) / 65535
    v = min_v + q_v * (max_v - min_v) / 65535
    payload size = 20 + 4*n bytes

A cluster with no UVs stores an EMPTY (data_size == 0) TVERTEXFRAMES node.
```

## Verification
- `python tools/mos_xmd.py --inventory <game>\Content\Models --check`:
  **631/631 files `ok`**, 0 failures, with the new UV cross-check enabled.
- Aggregate decode over all shipped models: **4662 clusters**, of which
  **2099 carry UVs** (size `20 + 4*n` tiling exactly, `n` == the paired
  `VERTEXFRAMES` vertex count) and **2563 store an empty node** (no UVs).
  **423,559** decoded UV pairs total; **0** bad clusters and **0** UV/vertex
  count mismatches.
- Decoded UVs are in a sane range (samples land in `0.00`..`1.00`; e.g.
  `CoordSys.xmd` cluster 0 bounds `[0.237, 0.200] .. [0.700, 0.700]`).
- `tools/test_mos_xmd.py` extended with headless synthetic-container tests:
  UV round-trip + dequantization, UV/vertex count mismatch, bad tiling, and the
  empty-chunk case.

## Consequences
- Every `.xmd` mesh's UVs are now extractable read-only alongside its geometry,
  so the native hand/arm mesh path of GH #46 / Option A of ADR-0413 has
  positions, normals, indices, and texture coordinates.
- The `0x201` (deferred) and `0` (raw) TVERTEX variants are recognised but not
  decoded; they do not occur in the shipped content.
- UVs are per-cluster: a cluster whose `TVERTEXFRAMES` node is empty has no
  texture coordinates and must be drawn untextured (or share a sibling
  cluster's UVs).

## Follow-ups
- Decode `BONEDEFORM`/`BONEINFLUENCE`/`BONEMATRIXMAP` (skinning), then extract
  the Riddick hand/forearm surfaces (`RIDDICK_ARMSCAR_*`, `Pa2_Hand_01`) with
  geometry + UVs + skin weights and add a native mesh path behind a flag,
  keeping `VRHandRenderer` as the fallback until a path is tested in-headset.
