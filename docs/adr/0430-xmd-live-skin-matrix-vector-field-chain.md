# ADR 0430: the live skin-matrix vector — corrected field chain and container

## Status
**Accepted (read-only correction)** — corrects the field-chain attribution that
ADR-0429 gave for the **live** per-bone skin-matrix array consumed by the
`.xmd` draw-side skinning (`FUN_100254e0` / `Cluster_TransformBones_*`,
ADR-0428). Verified against the MXR.dll disassembly. No game file was modified;
no code was deployed.

This continues the `#59 → #46` chain: ADR-0419 → 0423 → 0424 → 0425 → 0426 →
0428 → 0429 → **ADR-0430**.

## Context
ADR-0429 decompiled the provider *builder* `FUN_100238a0` and stated that the
live matrix array is reached as
`local_a0 = param_1[0x2c]; iVar15 = *(int*)(local_a0 + 0x14); matrices =
*(int*)(iVar15 + 0x18)`. Re-decompiling the same function (`FUN_1002dcc0`) and
reading its **disassembly** shows that `local_a0` is one decompiler local that is
**assigned twice**: once early (`param_1[0x2c]`, compared to `3` at two sites),
and again just before the skinned-cluster draw. The value that actually feeds the
matrix array is the **second** assignment.

## Finding (MXR.dll sha256 `06424f55…`, read-only, Ghidra 12.1.4)

### 1. The matrix-array owner is `param_1[0x25]` (`+0x94`), not `param_1[0x2c]` (`+0xb0`)
`FUN_1002dcc0` loads the early field and the late field into the **same** stack
slot `[EBP-0x9c]` (Ghidra's `local_a0`):

```
0x1002dd2a  MOV EAX, dword ptr [EAX + 0xb0]        ; EAX = param_1 -> param_1[0x2c]
0x1002dd30  MOV dword ptr [EBP + 0xffffff64], EAX  ; local_a0 = param_1[0x2c]   (early)
...
0x1002e41c  CMP dword ptr [EDI + 0x2c], 0x1        ; this+0x2c == 1           (EDI = this)
0x1002e467  TEST byte ptr [EBX + 0x4c], 0x1        ; param_1+0x4c             (EBX = param_1)
0x1002e47b  MOV EBX, dword ptr [EBX + 0x94]        ; EBX = param_1[0x25]
0x1002e481  MOV dword ptr [EBP + 0xffffff64], EBX  ; local_a0 = param_1[0x25]
0x1002e473  MOV ECX, dword ptr [EBP + -0x7c]       ; ECX = local_80 = param_6
0x1002e476  MOV ECX, dword ptr [ECX + 0x3c]        ; ECX = *(int*)(param_6 + 0x3c)
0x1002e479  CMP ECX, EDX                           ; EDX = 0
0x1002e487  JZ  0x1002e491
0x1002e489  MOV EBX, ECX
0x1002e48b  MOV dword ptr [EBP + 0xffffff64], ECX  ; local_a0 = *(int*)(param_6+0x3c)
```

So:

```
local_a0 = param_1[0x25];                 // = *(int*)(param_1 + 0x94)
if (*(int*)(param_6 + 0x3c) != 0)
    local_a0 = *(int*)(param_6 + 0x3c);   // override when non-null
```

`param_1[0x2c]` (`+0xb0`) is a **different, earlier** field of the same argument
(the pseudo compares it to `3`); `this+0x2c` is compared to `1` separately. It is
**not** the matrix-array owner.

### 2. The array itself (unchanged offsets)
Immediately after, the same slot is dereferenced — confirming ADR-0429's
`+0x14`/`+0x18`/`+0x04` shape but from the corrected base:

```
0x1002e577  CMP dword ptr [EBP + 0xffffff64], 0x0   ; if (local_a0 != 0) {
0x1002e593  MOV ECX, dword ptr [EBP + 0xffffff64]   ;   ECX = local_a0
0x1002e5a8  MOV EAX, dword ptr [ECX + 0x14]         ;   iVar15 = *(int*)(local_a0 + 0x14)
0x1002e5af  MOV EDX, dword ptr [EAX + 0x18]         ;   local_84 = *(int*)(iVar15 + 0x18)  (matrices)
0x1002e5be  MOV EAX, dword ptr [EAX + 0x04]         ;   local_94 = *(int*)(iVar15 + 0x04)  (count)
```

`local_84`/`local_94` are then pushed as arguments 2 and 3 to `FUN_100238a0`
(`0x1002f447 PUSH ECX; 0x1002f44b PUSH EAX; 0x1002f44c PUSH ESI; 0x1002f44d CALL
0x100238a0`), i.e. the array is handed to the per-cluster provider builder.

### 3. The container is a `TList_Vector<TMatrix43<float,float>>`-shaped object
The object at `iVar15` exposes `+0x04 = count` and `+0x18 = contiguous element
pointer`, with a `0x30`-byte element stride (the getter `FUN_10022f90`, ADR-0428,
indexes `matrices + idx * 0x30`). That is the layout of the shipped
`TList_Vector<TMatrix43<float,float>>` / `CListData` / `TMatrix43<float,float>`
RTTI family present in the binary:

- `.?AV?$TList_Vector@V?$TMatrix43@MH@@VTList_Vector_CNonAligned@@@@` @ `0x10169fc8`
- `.?AVCListData@?$TList_Vector@V?$TMatrix43@MH@@VTList_Vector_CNonAligned@@@@` @ `0x1016a1c0`
- `.?AV?$TMatrix43@MH@@` @ `0x1016a214`

The same `{+0x04 count; +0x18 data}` access pattern is used elsewhere in
`FUN_1002dcc0` for other `TList_Vector` fields (e.g. the `this+0x3c` list at
`0x1002f...`: `count = *(int*)(vec + 4)`, element = `*(int*)(*(int*)(vec + 0x18)
+ i*4)`), so it is the engine's generic list accessor convention, not a bespoke
skin struct.

## Decision / consequence
- **The live per-bone skin matrix array is reached from `param_1[0x25]`
  (`param_1 + 0x94`), overridden by `*(int*)(param_6 + 0x3c)`** — not from
  `param_1[0x2c]` as ADR-0429 recorded. The container is the engine's
  `TList_Vector<TMatrix43<float,float>>` (`+0x04` count, `+0x18` data, `0x30`-byte
  `TMatrix43` elements), which is exactly what `FUN_10022f90`'s `* 0x30` stride
  implies.
- The substantive conclusions of ADR-0428/ADR-0429 are **unchanged**: the
  provider composes no bind-inverse, and the matrices are read from a **live**
  engine-owned list. What changes is *which field* a follow-up probe must start
  from. For the native hand (#46/#58) the honest options remain a runtime read
  (GH #34) or an offline `bind_inverse × .xsa` pose.

### Not claimed
- Not claimed which class owns the object at `*(param_1 + 0x94)` (or
  `*(param_6 + 0x3c)`); only the field chain and its source argument.
- Not claimed whether the list elements are `world` or `world·bind_inverse`; only
  that `FUN_100238a0`/`FUN_10022f90` do not compose any bind-inverse.
- `param_1`/`param_6` argument identities are not resolved (the method is a
  virtual draw; `FUN_1002dcc0` is referenced only from the vtable slot
  `0x10144db8`, so it has no static caller to read the argument types from).
- The `SSE` variants were not analysed; findings are for `System/Win32_x86`.

## Change
- `docs/adr/0429-xmd-skin-matrix-provider-builder.md`: a **Correction** pointer
  to this ADR.
- `docs/rea/reference/MXR_SkinMatrixProvider.txt`: field chain corrected, with
  the disassembly excerpt.
- `docs/rea/mos-xmd-format.md`: the provider section's field chain + Next probe
  updated to `param_1[0x25]` / `*(param_6+0x3c)`.

## Evidence (2026-10-16, read-only)
- `rea function MXR.dll 0x1002dcc0` (`ev_…`, full pseudo + 2543-instruction
  disassembly saved at `tools/autoloop/logs/wake23_func_1002dcc0.json`): the
  `local_a0` double assignment, the `param_1[0x25]`/`param_6+0x3c` override, the
  `+0x14`/`+0x18`/`+0x04` reads, and the `FUN_100238a0` call site.
- RTTI strings `0x10169fc8` / `0x1016a1c0` / `0x1016a214` from the ADR-0429
  string search (`wake22_search_Matrix.json`).
- `rea xrefs MXR.dll 0x1002dcc0` → single reference `0x10144db8` (vtable slot).
- No C++/asset changed → `build-all.ps1`/CTest not rerun. Python verifiers:
  `test_mos_xmd.py`, `test_xmd_export.py`, `test_xmd_bake.py`, `test_xmd_pose.py`,
  `test_xsa_anim.py`, `validate_addresses.py`, `verify_hook_init.py`,
  `verify_reported_fixes.py`, `scenario_tests.py --check` all PASS.

## Files
- `docs/adr/0430-…md`, `docs/adr/0429-…md`,
  `docs/rea/reference/MXR_SkinMatrixProvider.txt`, `docs/rea/mos-xmd-format.md`
- cross-refs: ADR-0428/0429, GH #34/#46/#55/#58/#59
