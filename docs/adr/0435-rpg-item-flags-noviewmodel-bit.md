# ADR 0435: RPG Item `*flags` Bitfield — `noviewmodel` = bit 0x2 @ `+0x14c`

## Status
**Accepted (research record).** Advances GH #63 ("Duplicate hands: hide the
engine's first-person arms/hands when VR/native hands render"), **approach 1**
(locate the engine consumer of the `noviewmodel` bit and set/clear it at
runtime). This ADR records the flag encoding only; it changes no game or hook
behaviour.

## Context
GH #63 requires suppressing the engine's own first-person arms/hands while the
VR/native hands render. The two candidate routes are (1) drive the engine's
existing `noviewmodel` gameplay flag and (2) a per-draw viewmodel signature. A
prior wake (ADR-free issue comment on #63) established that `noviewmodel` is a
**shipped, per-item RPG `*flags` token**, not an engine-only symbol, but the bit
position and the owning field were unknown — so approach 1 could not even be
started.

REA/Ghidra 12.1.4 against `Content/GameClasses_Win32_x86.dll`
(sha256 `1898b736…`, the 2004 EFBB build) now resolves the encoding
(read-only; see `docs/rea/reference/GameClasses_RPG_ItemFlags.txt`).

## Decision
Record the RPG item flag bitfield and its token→bit mapping as a durable,
evidence-backed reference:

- The `*flags` property is parsed by the `CRPG_Object_Item` property registrar
  `FUN_100dfbd0` (`0x100dfbd0..0x100e0b31`), which resolves the property name
  `"FLAGS"` at `0x100e0568` and does
  `*(uint *)(item + 0x14c) |= FUN_100095a0(&PTR_s_wallcollision_101f7600)`.
  Injection of the mask is at `0x100e057a` (xref of the table base).
- `FUN_100095a0 → FUN_10009160` is the generic, reused name-set→bitmask
  translator: it walks a **NULL-terminated** `char*` table and sets
  `mask |= 1 << index` for every matched `a+b+c` token (numeric tokens are
  OR'd raw). Therefore **bit = 1 << (0-based index of the name in the table)**.
- The table base is `0x101f7600` (Ghidra symbol
  `PTR_s_wallcollision_101f7600`), **32 entries**, NULL-terminated at
  `0x101f7680`. Its first entries are `wallcollision, noviewmodel,
  playerlaserbeam, __equipped__, rpgmodelflags, aiming, playerflashlight, …`.

Consequently:

| field | value |
|---|---|
| RPG item flag bitfield | `CRPG_Object_Item + 0x14c` (u32) |
| `noviewmodel` | **bit 1 = `0x00000002`** |

Shipped-content cross-check (`Content/Registry/RpgAmmo.xrg`, 9/9 `*flags` lines):
`renderquantity+noviewmodel+needequippedlink+unselectable` →
`0x00100000 | 0x00000002 | 0x00010000 | 0x00000100 = 0x00110102`.

## Consequences
- Approach 1 now has its precondition: a runtime write to suppress the engine
  viewmodel would be `item->flags |= 0x2` at `CRPG_Object_Item + 0x14c` (and
  clearing it `&= ~0x2`). Locating the live **equipped item** pointer and the
  **draw gate** remains to be done; neither is claimed here.
- The bit table is directly reusable: the same reference gives every RPG flag
  bit (e.g. `wallcollision=0x1`, `aiming=0x20`, `__equipped__=0x8`,
  `rpgmodelflags=0x10`, `twohanded=0x800000`, `renderquantity=0x100000`).
- A candidate reader of the field: `0x10037111` (`test byte [eax+0x14c], 2`)
  inside `FUN_10037020`, whose bit tests (0x1/0x2/0x20) match this table; but
  the enclosing function is player-start/camera-entity selection, so it is **not
  established** as the viewmodel gate.

## Not claimed
- The **draw gate** that consumes `noviewmodel` is not identified.
- No in-headset validation (none is possible offline); this is a static,
  read-only finding.
- No change to shipped content — the mod must not write game assets; approach 1
  must write live engine state (or approach 2 must be adopted instead).
- `FUN_10037020`'s role is inferred (class-name entity query returning a 4×4
  transform + the low 16 flag bits); it needs its sole caller `FUN_10040aa0`
  decompiled to classify it.
