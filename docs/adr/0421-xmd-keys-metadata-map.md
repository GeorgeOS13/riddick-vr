# ADR 0421: `.xmd` `KEYS` is a string metadata map (it stores no bone rotations)

## Status
**Accepted** - the `KEYS` chunk of the shipped `MOS DATAFILE2.0` `.xmd` models is
pinned to the loader and decoded in `tools/mos_xmd.py`. It is a **flat string
key/value metadata map** (build provenance, texture and render options), **not**
an animation/bone-rotation store. Read-only analysis; no game file was modified
and no code is deployed.

This corrects the assumption inherited by ADR-0419 / ADR-0420 and the
`tools/xmd_export.py` / `tools/xmd_bake.py` notes ("bone rotations live in
`KEYS`/animation data"). It is part of GH #46 (real first-person hands), the
follow-on to the container (ADR-0414), geometry (ADR-0415), UVs (ADR-0416),
skinning (ADR-0417), skeleton (ADR-0418), export (ADR-0419) and bake (ADR-0420)
steps.

## Context
Baking the real Riddick arm/hand geometry (ADR-0420) produced a **bind-pose**
asset: `NODES` stores per-bone bind-pose model-space translations and a
parent/child hierarchy, but no rotations. The earlier roadmap reasoned that the
missing rotations could be read from the `KEYS` chunk ("skinned once bone
rotations from `KEYS` are decoded"). Before building a skinned draw path on that
premise it had to be verified.

`KEYS` is present in **1531** node records and in **631/631** shipped models, so
it is very common and worth pinning regardless.

## Finding - the loader chain
`KEYS` is read by the model reader, not by the geometry code:

1. `MXR.dll` `FUN_10083060` (the model reader, `CTriangleMeshCore`-adjacent) at
   `0x10083ce9`:

   ```
   CDataFile::GetNext(param_1, "KEYS");                 // 0x10149df0 == "KEYS\0"
   CKeyContainer::Read((CKeyContainer *)((int)this + 0x54), (CCFile *)*GetFile());
   ```

   `rea xrefs MXR.dll 0x10149df0` returns `{0x100823d7, 0x10083cbe}`, i.e. the
   writer `FUN_10081f30` and the reader `FUN_10083060`.

2. `MCCDyn.dll` `CKeyContainer::Read` (`0x100195d0`, exported; found via
   `rea search MCCDyn.dll "KeyContainer" --kind procedures`) reads:

   ```
   CCFile::ReadLE(&count);                       // u32
   for i in 0..count:
       CCFile::ReadLE(&key_len);                 // u32
       CCFile::Read(buf, key_len + 1);           // key text + NUL terminator
       CCFile::ReadLE(&value_len);               // u32
       if value_len == 0: value = ""             // nothing more is read
       else:            CCFile::Read(buf, value_len + 1);  // value + NUL
   ```

   The keys and values are stored as two parallel lists (`this+0x08`,
   `this+0x10`); it is a flat map, not a tree. (`CKeyContainerNode` /
   `ReadFromMemory_r` are a *separate* script tree and are not used here.)

## Finding - the on-disk layout
```
u32  count
count * {
    u32  key_len
    char key[key_len + 1]          (NUL terminated)
    u32  value_len
    char value[value_len + 1]      (NUL terminated; zero bytes when value_len == 0)
}
```
Note the asymmetry the loader implies: the key is always `key_len + 1` bytes,
but a **zero-length value consumes no bytes at all** (e.g. `NMSHAPE`). Navigating
the map with a naive `key_len`/`value_len` read misaligns on such entries.

## Aggregate evidence (all 631 shipped models)
`python tools/mos_xmd.py --inventory <game>\Content\Models --check` → **631/631
`ok`, 0 failures** (with the new `KEYS` tiling cross-check on).

| fact | value |
|---|---|
| `KEYS` chunks | **631** (every model, exactly one) |
| decoded entries | **4634** (max 33 in one model) |
| **distinct keys** | **54** |
| entries with an empty value | **319** |
| entries whose value contains control chars | **0** |
| `MODEL` == the file's own relative path | **606** |
| `MODEL` present but different (aliases / shared models) | **18** |
| no `MODEL` key | **7** |

The 54 keys are unambiguously asset/texture/render metadata — `XTXPATH` (631),
`MODEL` (624), `OGR_RENDERFLAGS` (608), `NORMALMAP_LOWRESMODEL` (334),
`NMSHAPE` (320), `NORMALMAP_FLAGS` (308), `NORMALMAP_GROW` (239),
`NORMALMAP_RES` (154), `GI_DIFFUSE_SOURCE`/`GI_SPEC_SOURCE`/`GI_MAP` (107/107/105),
`GI_LIGHT_*`, `OGR_ADDMODEL*`, `OGR_TOGGLEMODEL*`, `VERTEXSPLITANGLE`.
Sample (`Riddick_B05.xmd`):
```
MODEL           = Characters\Riddick\Riddick_B05.xmd
OGR_RENDERFLAGS = Triangles+PerPixel
XTXPATH         = S:\Models\Characters\Richard_B_Riddick\Riddick_B03.xtx
```
No key names or values resemble rotations/quaternions/keyframes.

### Shipped-data anomaly
One entry is **not** printable ASCII: `Characters\Dwellers\dweller.xmd` ships a
corrupt first key of 13 bytes (`9e 0c 90 0c 9e 0c …`) whose value is still the
model path. Because of this, the parser validates only the **structural**
(`exact tiling`) invariant, not printability; `dweller.xmd` is recorded here as a
corpus observation rather than a failure.

## Consequence
- `KEYS` is **not** the source of hand/arm bone rotations. `.xmd` carries
  bind-pose **translations only**; the missing rotations must come from the
  `.xsa`/`.xag` animation groups or from the engine's **live skeleton** at
  runtime (GH #34 territory).
- `tools/mos_xmd.py` now decodes the map (`decode_keys`, `KeyValue`,
  `Xmd.keys()/key_map()/has_keys()`, `--keys`), reports `keys_count`/`key_names`
  in `geometry_summary`, and cross-checks exact tiling in `validate()`.
- The `ROTATIONS_NOTE` constant, the module docstrings and ADR-0419/0420 are
  corrected accordingly.

## Unknowns / follow-ups
- **Where do rotations come from?** Two candidate paths, both out of scope here:
  (a) decode the `.xsa`/`.xag` animation-group format and evaluate a clip, or
  (b) read the engine's live per-bone matrices out of the game process
  (in-process; cannot be validated headlessly). Deciding between them is the next
  scoping step for the native hand draw path (GH #46).
  **Resolved toward (a)** by ADR-0422 / `tools/xsa_anim.py`: the `.xsa`
  `COMPRESSEDANIMATIONSET2` sets decode offline (pooled `QUATERNIONS` +
  `MOVES` + `TRACKS` + named `SEQUENCES`, `--check` 313/313 ok).
- The 18 `MODEL`-vs-relpath mismatches and 7 `MODEL`-less models are not
  itemised; they are alias/shared models and do not affect the layout.

## Evidence (commands)
- `rea xrefs MXR.dll 0x10149df0` → `[0x100823d7, 0x10083cbe]`
- `rea function MXR.dll 0x10083cbe` → `FUN_10083060` (calls `CDataFile::GetNext(param_1,"KEYS")` then `CKeyContainer::Read`)
- `rea search MCCDyn.dll "KeyContainer" --kind procedures` → `CKeyContainer::Read` @ `0x100195d0`
- `rea function MCCDyn.dll 0x100195d0` → `CKeyContainer::Read` (layout above)
- `python tools/mos_xmd.py --inventory <game>\Content\Models --check` → 631/631 ok
- `python tools/test_mos_xmd.py` → PASS (incl. 6 new `KEYS` tests)

## Files
- `tools/mos_xmd.py` - `KeyValue`, `decode_keys`, `Xmd.keys()/key_map()/has_keys()`,
  `geometry_summary` keys fields, `validate()` tiling cross-check, `--keys` CLI,
  module docstring.
- `tools/test_mos_xmd.py` - `test_keys_*` (round-trip, empty value, absent,
  bad tiling, count overrun, missing terminator, latin1 binary key).
- `tools/xmd_export.py`, `tools/xmd_bake.py`, `docs/rea/mos-xmd-format.md`,
  `docs/adr/0419-xmd-mesh-skin-export.md`,
  `docs/adr/0420-native-hand-mesh-asset.md` - corrected rotation-source notes.
- `docs/rea/reference/native_hand_mesh.json` - regenerated `rotations_note`.
