# ADR 0422: `.xsa` animation sets (`COMPRESSEDANIMATIONSET2`) are the offline source of per-bone rotations

## Status
**Accepted** - the shipped `.xsa` animation clips are `MOS DATAFILE2.0`
containers whose first child node, `COMPRESSEDANIMATIONSET2`, is a compressed
animation set (a pool of quantised quaternions + translation keys + per-track
descriptors + named sequences). The layout is pinned to the shipped `MXR.dll`
loader and decoded read-only by the new `tools/xsa_anim.py`.
`python tools/xsa_anim.py --inventory <game>\Content --check` → **313/313
animation sets `ok`, 0 failures** (23 non-animation `.xsa`/`.xag` files are
skipped: 3 legacy `ANIMATIONSET` clips and 20 `.xag` animation graphs).

This resolves the open question left by ADR-0421 ("where do hand/arm bone
rotations come from?") toward option (a): the animation groups are decodable
**offline**. No game file was modified and no code was deployed.

Part of GH #46 (real first-person hands) / GH #58 (rotation source for the
native draw path); builds on the container (ADR-0414), skeleton (ADR-0418) and
`KEYS`-is-metadata (ADR-0421) steps.

## Context
The native hand/arm mesh baked in ADR-0420 is bind-pose only: `.xmd` stores
per-bone bind-pose **translations** and the `KEYS` chunk is metadata
(ADR-0421). A skinned draw path therefore needs per-bone **rotations** from
somewhere else. `.xsa`/`.xag` animation groups ship with the game and are fully
offline-decodable; this ADR pins their container and recovery.

## Finding - the loader chain (`MXR.dll`, EFBB 2004 x86)
Discovered with `rea`/Ghidra 12.1.4. Class names come from the module's RTTI
strings (`.?AVCXR_Anim_Quatint16@@`, `.?AVCXR_Anim_CompressedTrack@@`,
`CXR_Anim::ReadCompressed`, ...).

1. **Dispatcher** `FUN_1001d7c0` probes, in order, `ANIMATIONSET`,
   `ANIMATIONTRACKSET`, `COMPRESSEDANIMATIONSET`, `COMPRESSEDANIMATIONSET2`
   (strings at `0x10144254`/`0x10144598`/`0x10144528`/`0x10144580`). For
   `COMPRESSEDANIMATIONSET2` it allocates a `CXR_Anim_Compressed` (vtable
   `0x101442b8`) and calls vtable slot `+0x20`.
   `rea xrefs MXR.dll 0x10144580` → `{0x1001d8e2, 0x10076d4e}`.

2. That slot is `0x10076d20` = **the compressed animation-set reader**
   (`.\XRAnimCompressed.cpp`). For each chunk it resizes a typed vector to the
   node's declared `count` and reads `element_size * count` bytes straight out
   of the file (`CCFile::Read`), in this order and into these object offsets:

   | chunk | object field | element |
   |---|---|---|
   | `QUATERNIONS` | `+0x18` | `CXR_Anim_Quatint16` (4 x i16) |
   | `MOVES` | `+0x20` | 3 x f32 |
   | `TRACKINDICES` | `+0x28` | u16 |
   | `QUATERNIONINDICES` | `+0x30` | u16 |
   | `KEYFRAMETIMES` | `+0x38` | f32 |
   | `TRACKS` | `+0x40` | `CXR_Anim_CompressedTrack` (2 x u16) |
   | `DATAKEYS` | `+0x48` | `CXR_Anim_DataKey` (variable) |
   | `SEQUENCES` | `+0x08` | `CXR_Anim_Sequence` (variable) |

   `QUATERNIONINDICES`, `KEYFRAMETIMES` and `TRACKS` are **required** by the
   reader; `DATAKEYS` and `SEQUENCES` must exist (payloads may be empty).

3. **Sequence records** `FUN_100774f0` (`CXR_Anim::ReadCompressed` →
   `FUN_1001a5e0`) read the on-disk record described below. Every shipped
   sequence uses inner version `0x103`.

4. `CXR_Anim_Quatint16` is the RTTI name that fixes the rotation encoding:
   four **signed 16-bit** components, `component / 32767` → a unit quaternion.
   `MOVES` are 3 x f32 **translations** in model units (e.g.
   `dialog_riddick.xsa` move 17 = `(1.40, 0.0, 56.85)`, on scale with the ~52
   unit Riddick figure).

## On-disk layout
Container: `MOS DATAFILE2.0` (identical to `.xmd`; `tools/mos_xmd.py`). The
root's first child is `COMPRESSEDANIMATIONSET2`; its children are the chunks.

```
QUATERNIONS        count * { i16 x, y, z, w }        ( / 32767 -> unit quaternion )
MOVES              count * { f32 x, y, z }           (translation, model units)
TRACKINDICES       count * u16
QUATERNIONINDICES  count * u16                        (per-key-slot index pool)
KEYFRAMETIMES      count * f32                        (keyframe times, seconds)
TRACKS             count * { u16 start; u16 flags }
DATAKEYS           variable-length records            (opaque here)
SEQUENCES          count * record                     (version 0x103)

SEQUENCES record (0x103):
    u32  version            = 0x103
    u16  field_a
    u16  field_b
    str  name               (u32 length + bytes, padded to 4)
    str  name2              (u32 length + bytes, padded to 4)
    u16  version2           = 0x100
    u16  track_start        (+0x20)  first TRACKINDICES slot of this clip
    u16  track_mid          (+0x22)  == track_start + rotation_count
    u8   rotation_count     (+0x24)
    u8   movement_count     (+0x25)
    u16  time_start         (+0x26)  first KEYFRAMETIMES slot
    u16  time_count         (+0x28)
    u16  extra_a            (+0x2a)
    u16  extra_c            (+0x2c)
```

For each fixed-size chunk `data_size == element_size * node.count` exactly.
`TRACKS[start].start` indexes `QUATERNIONINDICES` (never out of range).
`TRACKS` flags across all shipped tracks are exactly
`{0x0000, 0x0001, 0x8000, 0x8001}`: bit `0x0001` = the track carries movement
keys, bit `0x8000` = single/static key.

## Aggregate evidence (all 336 shipped `.xsa`/`.xag`)
`python tools/xsa_anim.py --inventory <game>\Content --check`:

| fact | value |
|---|---|
| animation sets decoded | **313** (`COMPRESSEDANIMATIONSET2`) |
| non-animation files skipped | **23** (3 legacy `ANIMATIONSET`, 20 `.xag`) |
| pooled quaternions | **2,710,465** |
| translation keys (`MOVES`) | **251,790** |
| tracks (`TRACKS`) | **110,046** |
| sequences (`SEQUENCES`) | **3,232** |
| keyframe times | **105,801** |
| **track `start` out of range** | **0 / 110,046** |
| `TRACKINDICES` max >= `TRACKS` count | **0 / 313** |
| fixed-chunk tiling violations | **0** |
| `SEQUENCES` tiling violations | **0** |
| sequence key windows (`TRACKINDICES`) not an exact partition | **0 / 313** |
| sequence time windows (`KEYFRAMETIMES`) not an exact partition | **0 / 313** |
| `track_mid != track_start + rotation_count` | **0 / 3,232** |
| pooled quaternion unit-norm \|norm-1\| | max **3.0e-5** (min norm 0.99997, max 1.00003) |

The **2.7M** quaternions all being unit length is the direct proof of the
4 x i16 `/32767` encoding; the sequence names are readable clip names
(1,676 distinct, e.g. `Gren_Active`, `Gren_Equip`, `Hangrail`, `Ladder`).

## Consequence
- The **rotation source for the native draw path is offline**:
  `tools/xsa_anim.py` decodes `QUATERNIONS` (the pooled rotations), `MOVES`,
  `TRACKS`, `KEYFRAMETIMES` and named `SEQUENCES` for any `.xsa`, with a
  structural `--check` that runs on the whole shipped corpus.
- `VRHandRenderer`'s native path can be fed real rotations without the running
  game, and the option-2 (in-process live skeleton, GH #34) route is no longer
  the only choice.
- `tools/xsa_anim.py` is the tool layer; the C++ consumer and the
  track→skeleton-node mapping are the remaining steps.

## Update (Wake 13) - the `SEQUENCES` table and the skeleton slot space

### Finding - `CXR_Anim_Sequence::Read` (`FUN_100774f0`, `.\XRAnimCompressed.cpp`)
After the base read (`CXR_Anim::ReadCompressed`, `FUN_1001a5e0`, inner version
`0x103`) the reader stores eight tail words straight into the object:
`+0x20` u16, `+0x22` u16, `+0x24` u8, `+0x25` u8, `+0x26` u16, `+0x28` u16,
`+0x2a` u16, `+0x2c` u16 (exactly the `<HHBBHHHH` tail `tools/xsa_anim.py`
already decoded). Corpus-derived roles:

| field | role | evidence (312/313 or 313/313 sets) |
|---|---|---|
| `+0x20 track_start` | first `TRACKINDICES` slot of the clip | `sum(rotation_count + movement_count) == TRACKINDICES.count` for **312/312** sets with tracks |
| `+0x22 track_mid` | `== track_start + rotation_count` | **313/313** sets |
| `+0x24 rotation_count` | rotation entries in this clip's key window | windows tile `TRACKINDICES` exactly (**313/313**) |
| `+0x25 movement_count` | movement entries in this clip's key window | as above |
| `+0x26 time_start` | first `KEYFRAMETIMES` slot | windows tile `KEYFRAMETIMES` exactly (**313/313**) |
| `+0x28 time_count` | time keys in the clip | as above |

Both window families are validated as **exact partitions** by the new
`window_tiling_problems` helper inside `AnimSet.validate()` (empty windows,
which several shipped clips carry, are ignored).

### Finding - the animation track space is the *skeleton* slot space
* `CXR_Skeleton::Read` = `FUN_100219d0` (`.\XRSkeleton.cpp`) reads `NODES`
  (32-byte stride), `NODEINDICES` (u16) and `ATTACHPOINTS`, then calls
  `FUN_1001ff60`, which walks every node's i16 at `+0x1c`/`+0x1e` and enforces
  **max rotation slot < 0x80 (128)** and **max movement slot < 0x20 (32)**
  (`"Too many rotation-slots used."` / `"Too many movement-slots used."`).
  These are the same `rotation_slot` / `movement_slot` fields pinned on `.xmd`
  `NODES` records in ADR-0418.
* Over the shipped corpus `max(TRACKINDICES) < TRACKS.count` in **312/312** sets
  and `max(TRACKS[i].start) < QUATERNIONINDICES.count` in **312/312** sets, so
  `TRACKINDICES` values index `TRACKS` and `TRACKS[i].start` indexes
  `QUATERNIONINDICES`. `TRACKS` therefore is a **per-key-block table**, not a
  bone-slot array: cutscene clips carry up to **1,333** `TRACKS` while the
  engine caps a skeleton at 128 rotation slots, so the clip→bone binding must go
  through the skeleton slot space rather than the raw track ordinal.
* The `EvalRot` entry in `XRAnimCompressed.cpp` (`FUN_10076680`) unconditionally
  throws (`"WTF is this used for?"`), i.e. the compressed class does not
  implement rotation evaluation itself - `COMPRESSEDANIMATIONSET2` is a storage
  format; the runtime path is the uncompressed `CXR_Anim_SequenceTracks`
  (`CXR_Anim_SequenceTracks::ReadData` = `FUN_1001b730`, whose 0x20-byte track
  records carry the slot words at `+0x1c`/`+0x1e`).

### Change
`tools/xsa_anim.py`: `Sequence.track_start/track_mid/rotation_count/
movement_count/time_start/time_count` + `key_window`/`time_window`;
`window_tiling_problems`; `validate()` now checks `track_mid`,
`TRACKINDICES`/`KEYFRAMETIMES` window tiling and `max(TRACKINDICES) <
len(TRACKS)`; new `--sequences` CLI. `tools/test_xsa_anim.py`: the fixture is now
shipping-consistent and 5 new tests cover the new invariants (14 total).

### Evidence (commands)
- `rea function MXR.dll 0x100774f0` -> `CXR_Anim_Sequence::Read`
- `rea function MXR.dll 0x100219d0` -> `CXR_Skeleton::Read` (NODES/NODEINDICES/
  ATTACHPOINTS -> `FUN_1001ff60`)
- `rea function MXR.dll 0x1001ff60` -> rotation/movement slot finaliser (caps
  0x80/0x20)
- `rea function MXR.dll 0x1001b730` -> `CXR_Anim_SequenceTracks::ReadData`
- `rea function MXR.dll 0x10076680` -> `EvalRot` stub (`"WTF is this used for?"`)
- `python tools/xsa_anim.py --inventory <game>\Content --check` -> **313/313 ok,
  0 FAIL**
- `python tools/test_xsa_anim.py` -> PASS (14 tests)

### Still open
`rotation_count`/`movement_count` are proven to *partition* `TRACKINDICES`, but
the meaning of the individual `TRACKINDICES` entries (which of the two pool
words each key consumes, and how a `TRACKS` block binds to a `NODES` slot) is the
next step; the data is consistent with "per-key owning track index", and
`rotation_slot == node index` for used `.xmd` nodes supplies the slot order.

## Unknowns / follow-ups
- **Track -> skeleton-node resolution is not yet end-to-end.** `TRACKS[i].start`
  is a valid `QUATERNIONINDICES` slot (proven, 312/312) and the skeleton's
  rotation/movement slot space is pinned (above), but the exact
  `TRACKS`-block -> `NODES`-slot mapping for cutscene clips (`TRACKS` up to
  1,333) is unresolved; the second `TRACKS` word is a flag word
  (`0x8000` marks single-key tracks), not a count.
- `Grenade.xsa` carries a **1-entry** `QUATERNIONS` pool while
  `QUATERNIONINDICES` reaches 23 (one of **7** sets where
  `max(QUATERNIONINDICES) >= QUATERNIONS.count`), so the pool index is *not*
  always a plain 1:1 `QUATERNIONS` index; the other 305 sets are consistent with
  1:1. Cracking the 7 exceptions (per-clip pool re-base?) is what turns the pool
  into a posed skeleton.
- `DATAKEYS` (`CXR_Anim_DataKey::Read`) records are variable-length; the parser
  exposes their size/count but does not decode the fields.
- Only sequence version `0x103` is shipped; `FUN_1001a5e0` also defines
  `0x100`/`0x101`/`0x102`, which the tool rejects explicitly.

## Evidence (commands)
- `rea search MXR.dll "COMPRESSEDANIMATIONSET2"` → `0x10144580` (+ error strings)
- `rea xrefs MXR.dll 0x10144580` → `{0x1001d8e2, 0x10076d4e}`
- `rea function MXR.dll 0x1001d8e2` → dispatcher `FUN_1001d7c0`
- `rea function MXR.dll 0x10076d4e` → reader `FUN_10076d20` (`XRAnimCompressed.cpp`)
- `rea read-bytes MXR.dll 0x101442b8` → vtable; `+0x20 = 0x10076d20`
- `rea function MXR.dll 0x100774f0` → `CXR_Anim::ReadCompressed` / sequence read
- `rea function MXR.dll 0x1001a5e0` → `CXR_Anim` base read (versions 0x100..0x103)
- `rea search MXR.dll "CXR_Anim"` → RTTI incl. `CXR_Anim_Quatint16`, `CXR_Anim_CompressedTrack`
- `python tools/xsa_anim.py --inventory <game>\Content --check` → 313/313 ok
- `python tools/test_xsa_anim.py` → PASS

## Files
- `tools/xsa_anim.py` - `AnimSet`, `Track`, `Sequence` (named
  sequence-table fields + `key_window`/`time_window`), `window_tiling_problems`,
  `decode_quaternions`/`decode_moves`/`decode_tracks`/`decode_sequences`,
  `AnimSet.validate()`, `--inventory`/`--check`/`--tracks`/`--sequences`/
  `--quaternions` CLI.
- `tools/test_xsa_anim.py` - synthetic-container tests (round-trip, tiling,
  non-unit quaternion, unknown flags, track range, missing chunk, sequence
  tiling, unsupported sequence version, sequence key/time window gap/overlap,
  `track_mid` mismatch, `TRACKINDICES` range).
- `docs/rea/mos-xmd-format.md` - `.xsa` animation-set section (cross-ref).
- `.github/workflows/vr-worker.yml` - runs the new test next to the `.xmd` tests.
- `docs/adr/0421-xmd-keys-metadata-map.md` - "rotations" follow-up now points here.
