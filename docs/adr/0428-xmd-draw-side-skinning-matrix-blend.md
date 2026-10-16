# ADR 0428: the `.xmd` draw-side skinning is a plain matrix blend (no per-cluster reference transform)

## Status
**Accepted (read-only finding)** — resolves the last open candidate of GH #59
("a runtime per-cluster matrix built by the model/skin class"). The draw-side
skin function does **not** read any per-cluster reference/bind-inverse transform
from the mesh: it blends matrices supplied entirely by a second object (the live
matrix provider). No game file was modified and no code was deployed.

This closes the `#59` chain that ran through ADR-0419 (mesh/skin export),
ADR-0423 (`BONEDEFORM` bone-index palette), ADR-0424 (cluster pose ≠ `NODES`
bind), ADR-0425 (`CLUSTER`/`MISCELLANEOUS` is a reference box, not a matrix) and
ADR-0426 (`BONEMATRIXMAP`/`matrix_map_id` is a per-cluster id, not a key).

## Context
`#59` assumed a "per-cluster reference/bind-inverse transform" had to be found,
because the stored cluster geometry is **not** the `NODES` bind pose (ADR-0424:
the Riddick arm clusters are disjoint from the body and hold a raised pose).
ADR-0425 and ADR-0426 removed every in-file candidate. The only remaining one
was a runtime matrix built by the model/skin class from the live skeleton. This
wake decompiled that consumer.

## Finding (MXR.dll sha256 `06424f55…`, read-only, Ghidra 12.1.4)
- The warning string
  `WARNING: (CXR_Model_TriangleMesh::Cluster_TransformBones_V_N_TgU_TgV) Missing vertex component.`
  lives at MXR.dll **VA `0x10144a1d`** (file offset `0x143837`, `.rdata`).
  `rea search MXR.dll Cluster_TransformBones` reports it once (at `0x10144a11`).
  There is **no** procedure named `Cluster_TransformBones*` in the symbol table
  (`rea search … --kind procedures` → empty; the PDB is not loadable), so the
  method had to be found from its warning.
- Its only code reference is `0x10025cf5`, inside **`FUN_10025ced`
  (`0x10025ced..0x10025d13`, 39 bytes)** — the cold block that formats and writes
  the warning (`FUN_10005990` + `ConsoleWrite`).
- `rea xrefs MXR.dll 0x10025ced` → **four call sites** `0x10025595`,
  `0x1002559d`, `0x100255a5`, `0x100255ad`, all inside **`FUN_100254e0`**
  (body `0x100254e0..0x10025cec`, non-contiguous; Ghidra split the warning tail
  into its own function). So the method is **`FUN_100254e0`**; its only caller is
  `0x1002dcc0`.
- `FUN_100254e0(void* mesh, void* provider)` (see
  `docs/rea/reference/MXR_ClusterTransformBones.txt`):

| role | field / call | decoded shape |
|---|---|---|
| mesh vertex count | `FUN_1007e770(mesh)` | same call `CreateMatrixIW` makes (ADR-0426) |
| per-vertex influence records | `FUN_1007f9f0(mesh)` | stride 4; `count` byte + `start` u16 |
| influence pool | `FUN_1007fa40(mesh)` | 8-byte entries `{u16 bone; u16 pad; f32 weight}` (weight at `+4`) |
| source components | `FUN_1007ee20` / `FUN_1007f100` / `FUN_100825a0` / `FUN_10082750(mesh)` | V / N / TgU / TgV |
| destination components | globals `DAT_10172204/08/0c/10` | V / N / TgU / TgV out |
| **per-bone matrix** | **`FUN_10022f90(provider, bone)`** | `const float*` = **3×4 (12-float) affine** (see below) |

- Per vertex it reads `count` and `start`, then:
  - **`count == 1`** → `v' = M·v` (position uses `M[9..11]` translation; N/TgU/TgV
    use the 3×3 part only) — a single fetched matrix, applied directly;
  - **`count == 2`** → the two matrices are **weight-blended inline** by their pool
    weights, then applied;
  - **`count > 2`** → a **linear weighted sum** `Σ_k w_k·M_k` (row-wise) is
    accumulated and applied once (the `≤ 4` cap `CreateMatrixIW` enforces).
- If any of the four source arrays is null it emits the "Missing vertex component"
  warning and skips. No other mesh field is consulted.

**The mesh is read only for geometry, the influence records and the influence
pool. No bind / reference / inverse matrix is read from the mesh, the cluster,
its `MISCELLANEOUS` header, or `matrix_map`/`matrix_map_id`.**

### The matrix provider (`FUN_10022f90`) — `{float* matrices; u8* palette}`
```c
int __thiscall FUN_10022f90(void* this, int bone) {          // MXR.dll 0x10022f90
    if (*(int*)((int)this + 4) != 0)                          // this+0x04 = u8* palette?
        return (uint)*(byte*)(*(int*)((int)this + 4) + bone) * 0x30 + *(int*)this;
    return bone * 0x30 + *(int*)this;                          // this+0x00 = float* matrices
}
```
- The provider is an object with `+0x00` = a **per-bone 3×4 matrix array of `0x30`
  (48) bytes per entry** and `+0x04` = an **optional byte palette** that remaps the
  influence's `bone` to a matrix index; absent (`NULL`) the bone index is used
  directly. Its callers are `0x10024ca0` and `0x100254e0`.
- Call site (in the model draw method **`FUN_1002dcc0`**, `0x1002dcc0..0x1002f16c`
  + `0x1002f170..0x1002ffad`, 47.7 k chars, 64 callees, 23 strings, no direct
  callers → reached via vtable): `FUN_100254e0(pfVar6, DAT_1016a7dc)`, i.e. the
  provider is the **global pointer `DAT_1016a7dc` (`= &DAT_1016a7d0 + 0x0C`)**,
  a renderer-level object set outside this function; the sibling path calls
  `FUN_10024ca0(pfVar6, DAT_1016a7dc)` on the same provider.
- So the runtime palette concept (a "matrix map") exists **in the provider**, not
  only in the mesh. Whether the matrices are `world` or `world·bind_inverse` is
  decided when the provider is built — this getter only indexes. **Next probe: the
  builder of `DAT_1016a7d0`/`DAT_1016a7dc`.**

## Decision / consequence
- GH #59's premise is resolved as a **negative**: the per-cluster
  reference/bind-inverse transform **does not exist on the `.xmd` draw path**.
  The per-vertex transform is `Σ_k w_k · M(bone_k)` where every `M` comes from the
  second argument (the runtime matrix provider) via `FUN_10022f90`.
- Given ADR-0424 ("stored geometry ≠ `NODES` bind"), the provider's matrices are
  therefore **not** the plain `.xmd` `NODES` translations — the reference must be
  built where the provider is built (or the paired provider is an animated
  skeleton). That is the only place left for it.
- **Narrowed next probe:** the provider is now decoded down to
  `{float* matrices; u8* palette}` with `0x30`-byte (3×4) entries, so the
  reference/bind-inverse (if any) is baked **into the provider's matrices when
  they are built**. The next probe is therefore:
  1. who builds the provider's matrix array, and does that step multiply in a
     bind-inverse (`world · bind_inverse`) or store raw `world` matrices (probe
     the builder of the global `DAT_1016a7d0` / `DAT_1016a7dc`); and
  2. the model draw method `FUN_1002dcc0` itself (47.7 k chars) for how it selects
     the V/N/TgU/TgV destination pass and pairs the mesh with the provider.
- For the native hand draw paths (#46/#59) this means a *correct* skinned draw
  cannot be derived from `.xmd` alone: it must read the provider's matrices at
  runtime (`#34`) or keep the rigid draw (`ADR-0427`). It also explains why a
  naive "bind-inverse from `NODES`" reconstruction would look wrong.

### Not claimed
- Not claimed that the provider matrices are `world · bind_inverse`; only that
  the getter indexes a `0x30`-byte array through an optional byte palette.
- Not claimed which class the provider is; only that `FUN_100254e0` receives it as
  the global pointer `DAT_1016a7dc` (field `+0x0C` of the object at
  `DAT_1016a7d0`) from the model draw method `FUN_1002dcc0`, and that
  `FUN_10022f90` is `(provider, bone) -> 3×4 matrix` with the
  `{float* matrices; u8* palette}` layout above.
- The four call sites are the missing-component guards; the ADR does not claim
  anything about their operand order beyond "one per component".

## Change
- `docs/rea/reference/MXR_ClusterTransformBones.txt` (new): the reference
  pseudocode of `FUN_100254e0`, its matrix getter `FUN_10022f90` and the
  `FUN_1002dcc0` call site, with the field/call table above.
- Corrected stale "the baked asset is the artist bind pose" wording, which
  ADR-0424 had already disproved for the arm clusters:
  `src/hook/native_hand_mesh.h`, `tools/xmd_bake.py`, `docs/adr/0420-…md`.
- `docs/rea/mos-xmd-format.md`: "Next steps" now points at `FUN_10022f90` /
  `0x1002dcc0` instead of the (now-closed) per-cluster transform hunt.

## Evidence (2026-10-16, read-only)
- `rea search "C:\Program Files (x86)\Starbreeze Studios\Riddick EFBB\System\Win32_x86\MXR.dll" Cluster_TransformBones`
  → one string at `0x10144a11` (`WARNING: (CXR_Model_TriangleMesh::Cluster_TransformBones_V_N_TgU_TgV) Missing vertex component.`, `0x10144a1d`).
- `rea search MXR.dll Cluster_TransformBones --kind procedures` → **empty**
  (no symbol; the PDB is not loadable).
- Raw byte scan of all four `System/Win32_x86` DLLs: the string is present only in
  **MXR.dll** (file offset `0x143837`); `RndrGL.dll` has none.
- `rea function MXR.dll 0x10025cf5` → `FUN_10025ced` (the warning emitter);
  evidence id `ev_b39237005178ece323b1c392d7e711ad856364d2476b240157d54e540bc05210`.
- `rea xrefs MXR.dll 0x10025ced` → `[0x10025595, 0x1002559d, 0x100255a5, 0x100255ad]`;
  evidence id `ev_a8855f404bbb49acc59f018f9dc096e71aea95609db6ef2a0a52deb1cd857c8f`.
- `rea function MXR.dll 0x10025c00` → `FUN_100254e0` (`0x100254e0..0x10025cec`,
  ranges `…5aed / 5af0..5bee / 5bf0..5cec`), 1 caller `0x1002dcc0`;
  evidence id `ev_bd1a98804574a4f32ad2943cc6f8902daf38e9afc5d74e2f988d60b4e81c259b`.
- `rea function MXR.dll 0x10022f90` → `FUN_10022f90 thiscall (void* this, int
  bone)`, callers `0x10024ca0`, `0x100254e0`; body = the `0x30`-stride matrix
  array + optional byte palette quoted above.
- `rea function MXR.dll 0x1002dcc0` → `FUN_1002dcc0 thiscall` (model draw method,
  `0x1002dcc0..0x1002f16c` + `0x1002f170..0x1002ffad`, 64 callees, no direct
  callers); it calls `FUN_100254e0(pfVar6, DAT_1016a7dc)` and
  `FUN_10024ca0(pfVar6, DAT_1016a7dc)` with the global provider pointer.
- `cmake --build build/x86 --config Release` → **opengl32.dll** built;
  `ctest --test-dir build/x86 -C Release` → **5/5 PASS**; same on **build/x64**.
- `python tools/xmd_bake.py --cpp src/hook/native_hand_mesh_data.h --verify` →
  OK (4 meshes, 1548 verts) — the comment-only header edit did not touch the
  generated asset.
- `test_mos_xmd.py` (50), `test_xmd_export.py`, `test_xmd_bake.py`,
  `test_xmd_pose.py` (7), `test_xsa_anim.py` (19) all PASS.

## Files
- `docs/rea/reference/MXR_ClusterTransformBones.txt`, `docs/adr/0428-…md`,
  `src/hook/native_hand_mesh.h`, `tools/xmd_bake.py`, `docs/adr/0420-…md`,
  `docs/rea/mos-xmd-format.md`
- cross-refs: ADR-0419/0423/0424/0425/0426/0427, GH #34/#46/#55/#58/#59
