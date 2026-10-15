# ADR 0414: MOS DATAFILE2.0 container layout recovered from the shipped loader

## Status
**Accepted** — layout confirmed against the decompiled engine loader and
re-validated across all 631 shipped `.xmd` models. Read-only analysis; no game
file was modified and no code is deployed.

This **corrects** the byte layout described in ADR-0413 §A (`header 0x30`,
`node name[16] + u32[8]`, `+0x18` node-table start, `+0x20` data start). The
parser observed those fields correctly by dereference, but the *field split* was
wrong: the node name field is 24 bytes (`0x18`), not 16, and the six engine
fields begin at `+0x18`.

## Context
GH #55 requires decoding the real Riddick hand/forearm geometry and skinning out
of the `MOS DATAFILE2.0` `.xmd` containers (GH #46). Ghidra 12.1.4 is installed
and wired into `rea`, so the loader can be read statically.

## Evidence (2026-10-15, read-only; Ghidra 12.1.4 via `rea`)
Target: `Content/../System/Win32_x86/MCCDyn.dll`
(sha256 `1595e797…43618d8`; PDB `Dll_MCC.pdb`).

- The literal magic string `"MOS DATAFILE2.0\0"` lives at **RVA `0x1005C298` /
  file offset `0x5B698`** (segment `.rdata`).
- Incoming references: `0x100330C0`, `0x100330CC`, `0x10034053`, `0x100340F6` —
  i.e. exactly two functions:
  - `CDataFile::Open` @ **`0x10032FB0`** (reads + version-checks the container),
  - `CDataFile::Create` @ **`0x10033F10`** (writes the 4×(4-byte) magic words
    `"MOS " / "DATA" / "FILE" / "2.0"` into the root node at `+0x08..+0x17`,
    confirming the root node's `0x18`-byte name field *is* the magic).
- `CDataFile::Open` decompilation:
  - `CCFile::Read(file, buf, 0x10)` → 16-byte magic;
  - `CCFile::ReadLE ×4` → four `u32` at file offsets `0x10,0x14,0x18,0x1C`
    (read but unused — they overlap the root record's name tail/sibling/child);
  - compare buf vs `"MOS DATAFILE1.0"` → version 1, vs `"MOS DATAFILE2.0"` →
    version 3, else throw "Not a valid datafile.";
  - for v2.0: `CCFile::Seek(file, 0)` then `CDataFile::ReadTree_r(root, NULL)`.
- `CDataFile::ReadTree_r` @ **`0x10032840`** decompilation (node record shape):
  - `pos = CCFile::Pos()` (absolute record offset) → `node+0x38`;
  - `CCFile::Read(file, node+0x08, 0x18)` → a **0x18-byte (24-byte) name field**;
  - `CCFile::ReadLE ×6` → six `u32` at `node+0x20..node+0x37`
    (on disk `record+0x18..+0x2C`);
  - recursion uses `node+0x24` (= disk `+0x1C`) as the **first child** offset and
    the loop uses `node+0x20` (= disk `+0x18`) as the **next sibling** offset;
  - each node = `0x18` name + `6×u32` = **`0x30`-byte record**.

## Recovered layout
```
file  : the ROOT node record is at file offset 0. Its 0x18-byte name field
        holds "MOS DATAFILE2.0\0" (NUL padded), so the loader's 16-byte magic
        compare reads the root node's name. There are no separate header fields.

node  : 0x30-byte record
    +0x00  char[0x18]  name (NUL padded)
    +0x18  u32         next sibling node offset (0 = none); root's == first node
    +0x1C  u32         first child node offset    (0 = none)
    +0x20  u32         data offset (absolute file offset); root's == data start
    +0x24  u32         data size in bytes
    +0x28  u32         element/frame count
    +0x2C  u32         flags (e.g. TVERTEXFRAMES = 0x201)

data  : chunk payloads. For every shipped model the payload regions tile
        [root.data_offset, EOF) exactly (no gaps, no overlaps).
```
The tree is walked exactly as `ReadTree_r` does: from the root, descend into the
child subtree, then continue along the sibling chain. `node_start`/`data_start`
previously read as raw header `u32`s are just `root.next_sibling` /
`root.data_offset`.

## Verification
- `tools/mos_xmd.py` reworked to the recovered layout (name `0x18` + 6 fields),
  plus a `validate()` self-check (root/header agreement, pointer integrity,
  linked-walk coverage, exact payload tiling) and a `--check` mode.
- `python tools/mos_xmd.py --inventory <game>\Content\Models --check`:
  **631/631 files `ok`**, 0 failures.
- Regression: `--inventory` output (triangles, bones, sizes) is byte-identical
  between the previous parser and this one for all 18 `Characters/Riddick`
  models — the correction changes documentation/robustness, not the parsed
  geometry numbers that ADR-0413 already relied on.
- `tools/test_mos_xmd.py` builds a synthetic container (round-trip + corrupt
  pointer + bad magic) and passes headlessly without the game install.

## Consequences
- The container is now fully specified for the next step of GH #55: decode the
  per-chunk payloads (`VERTEXFRAMES`, `TVERTEXFRAMES`, `BONEDEFORM`, `NODES`,
  `TRIANGLES`) whose offsets/sizes this layout addresses.
- Names may exceed 16 bytes, so the old 16-byte read was a latent bug; the parser
  now reads the true 24-byte field.
- No engine-offset behaviour changes; target build still to be pinned (GH #56).

## Follow-ups
- **Done — ADR-0415.** The chunk payload readers were decompiled: the
  `VERTEXFRAMES` (positions/normals) and `TRIANGLES` layouts are recovered and
  validated across all 631 models. `TVERTEXFRAMES` (UVs) and
  `BONEDEFORM`/`BONEINFLUENCE`/`BONEMATRIXMAP` (skinning) remain.
- Keep the procedural `VRHandRenderer` as the fallback until a native mesh is
  tested in-headset (GH #46).
