# Starbreeze `MOS DATAFILE2.0` (`.xmd`) format — REA notes

Read-only reverse-engineering of the Riddick model container, so the real
character/hand meshes and skeletons can be reused for VR (GH #55, ADR-0413).
Nothing here writes to the game install.

Tooling: **REA 6.5.0 + Ghidra 12.1.4** (`GHIDRA_INSTALL_DIR` set). Full
Ghidra pseudocode excerpts live in `docs/rea/reference/`.

## Artifacts analysed
| Module | Build | SHA-256 | Role |
|---|---|---|---|
| `MCCDyn.dll` | AoDA (F:) | `fa4c995cb066626e1fb3d85adfbc965efea4a92e724a72c6b7ae135a1de58aee` | generic `CDataFile` container read/write |
| `MXR.dll` | EFBB (C:) | `06424f55f5a0f85eb09b5fbd69491c7f6a88de808c26c513e8dac180f3c2052d` | model/skeleton/mesh (`CTriangleMeshCore`) |
| `MXR.dll` | AoDA (F:) | 4,596,976 B | same chunk names present |

Only `MCCDyn.dll` contains the literal `MOS DATAFILE2.0` (`0x100e808c`) and
`MOS DATAFILE1.0` (`0x100e80b4`). Only `MXR.dll` contains the geometry chunk
names `VERTEXFRAMES`, `TVERTEXFRAMES`, `BONEDEFORM`, `SKELETON`, `NODEINDICES`.

## Container (`CDataFile`)
- Writer: `CDataFile::Create` — AoDA `MCCDyn.dll` xref `0x10082a74`.
- Reader: `CDataFile::ReadTree` — AoDA `MCCDyn.dll` xref `0x10082f15`
  (`?ReadTree@CDataFile@@AAEXPAVCCFile@@@Z`). Pseudocode:
  `docs/rea/reference/MCCDyn_CDataFile_ReadTree.txt`.
- Reads a **0x30-byte header**, matches `MOS DATAFILE2.0` (v2) / `1.0` (v1),
  seeks back to the node-table start, and reads **N × 0x30-byte node records**
  (N from header+0x28, or counted recursively when 0).
- Header: `+0x18` node-table start (`0x30`); `+0x20` data-region start;
  `+0x28` u32 declared node count (0 ⇒ count recursively).
- Node record (0x30), validated by `tools/mos_xmd.py`:
  | off | meaning |
  |---|---|
  | `+0x00` | name[16], NUL-padded |
  | `+0x10` | u32[2] flags/reserved (usually 0) |
  | `+0x18` | u32 node-link A (offset) |
  | `+0x1C` | u32 node-link B (offset) |
  | `+0x20` | u32 data offset (absolute) |
  | `+0x24` | u32 data size |
  | `+0x28` | u32 element count / scalar |
  | `+0x2C` | u32 flags (e.g. `TVERTEXFRAMES` = `0x201`) |
- `ReadTree` converts A and B to node indices via `(value - nodeTableStart)/0x30`
  and links them. One is the first child, the other the next sibling; the
  decompiler names vs. the parser labels differ, but `tools/mos_xmd.py`
  (`+0x18`=next_sibling, `+0x1C`=first_child) reproduces the correct
  per-cluster nesting. **Exact role confirmation is a small pending task.**
- Chunk payloads tile the data region contiguously — verified for every
  `TRIANGLES` (declared count == node `count`).
- `CDataFile::GetNext(df, "NAME")` selects a sibling chunk by name; that is how
  the mesh reader finds `VERTEXFRAMES`/`TVERTEXFRAMES`/`BONEDEFORM`.

## Geometry (`CTriangleMeshCore`) — in `MXR.dll`
- Reader entry: `CTriangleMeshCore::Read` ≈ `FUN_10080f50` (error strings
  `"Corrupt model. (1|2)"`), referenced from `0x10081222`. Pseudocode:
  `docs/rea/reference/MXR_CTriangleMeshCore_Read.txt`.
- It calls, in order:
  - `GetNext("VERTEXFRAMES")` → `FUN_1007fba0` (sizes the vertex-frame list)
  - `GetNext("TVERTEXFRAMES")` → `FUN_1007fce0`
  - `GetNext("BONEDEFORM")` → `FUN_1007d260`; the chosen data offset is stored
    at mesh `+0x94`.
- Chunk-name xrefs: `BONEDEFORM`@`0x10149c88` → `0x100817f6` (reader) /
  `0x1007daaf` (writer); `VERTEXFRAMES`@`0x10149ca4` → `0x10081222`;
  `TVERTEXFRAMES`@`0x10149c94` → `0x10081437`.
- `FUN_1007d260` (`MXR_ReadBoneDeform.txt`) — BONEDEFORM layout (observed):
  1. seek to mesh+0x94; read an int (`FUN_10006620`) = **bone-record count**;
  2. resize the list at mesh+0x14, then `CCFile::Read` a raw array of
     `count × stride` bytes;
  3. read a second int = **bone-index-list count**; resize mesh+0x1c;
  4. `FUN_1007c150(file, 0x200, …)` reads that secondary array;
  5. per primary record: `u16 index = *(u16*)(rec+2)`, and byte `rec+1` = number
     of bone influences; the secondary list holds 8-byte entries whose first
     `u16` is a 1-based bone index.
  → i.e. per-vertex skin influences. Exact stride/packing to be finalised.
- `VERTEXFRAMES` positions are byte-packed (not plain float3 arrays); the
  reader sizes the list then fills it in `CTriangleMeshCore::Read`. The precise
  packing (likely per-frame deltas) is the remaining decode target.

## Reproduce
```
$env:GHIDRA_INSTALL_DIR="C:\Users\GigelCastel\Tools\ghidra_12.1.4_PUBLIC"
rea search "<game>\System\Win32_x86\MXR.dll" "BONEDEFORM" --format json
rea xrefs  "<game>\System\Win32_x86\MXR.dll" --address 0x10149c88 --format json
rea decompile "<game>\System\Win32_x86\MXR.dll" --procedure 0x10080f50 --format json
rea decompile "<game>\System\Win32_x86\MCCDyn.dll" --procedure 0x10082f15 --format json
```
Container-level inventory without a decompiler: `python tools/mos_xmd.py --inventory <Content/Models>`.

## Next steps (for the loop/next agent)
1. Finish the `VERTEXFRAMES` packing and `BONEDEFORM` stride by reading the
   remainder of `CTriangleMeshCore::Read` + `FUN_1007d260` and validating
   against `tools/mos_xmd.py` counts (e.g. Riddick_B03: 74 bones, 2925 skin
   records, 520 tris in cluster 1).
2. Locate hand/forearm bones: the skeleton is `SKELETON`→`NODES` (34 B/node).
   Bone **names** are not stored as strings; recover the hash/name scheme from
   the `.xsa/.xag/.xrg` animation groups or the MXR skeleton reader.
3. Emit an OBJ (positions + triangles + UVs) for `Riddick_B05`
   (`RIDDICK_ARMSCAR_*`) and `Pa2_Hand_01`, then a skinned draw path in
   `VRHandRenderer` behind a config flag (fallback: current procedural hands).
   **Done:** OBJ/JSON export = ADR-0419; baked native C++ asset + cluster→surface
   mapping = ADR-0420 (`tools/xmd_export.py`, `tools/xmd_bake.py`,
   `src/hook/native_hand_mesh.h`). Remaining: the `VRHandRenderer` draw path
   itself (rigid first, then skinned once bone rotations from `KEYS` are known).
