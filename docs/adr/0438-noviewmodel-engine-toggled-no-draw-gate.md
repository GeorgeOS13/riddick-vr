# ADR 0438: `noviewmodel` is Engine-Toggled, With No Direct Draw-Gate Reader

## Status
**Accepted (research record + direction change).** Advances GH #63 ("Duplicate
hands: hide the engine's first-person arms/hands when VR/native hands render").
Records a static, read-only finding and **re-directs** #63 from approach 1 to
approach 2. No game or hook behaviour is changed by this ADR.

## Context
GH #63 needs the engine's own first-person arms/hands suppressed while the
VR/native hands render. ADR-0435 pinned the encoding — `noviewmodel` is **bit 1
(`0x00000002`) of the RPG item `*flags` bitfield at `CRPG_Object_Item + 0x14c`**
— and left approach 1 ("locate the engine consumer of the bit and set/clear it
at runtime") blocked on two unknowns: the live equipped-item pointer and the
**draw gate**.

This ADR resolves the second unknown, negatively.

## Finding (read-only; REA/Ghidra 12.1.4)
Evidence and reproductions: `docs/rea/reference/GameClasses_NoviewmodelRuntime.txt`.

1. **The engine sets and clears the bit itself.** `FUN_100e6aa0`
   (`GameClasses_Win32_x86.dll` `0x100e6aa0..0x100e6b3c`, reached only through
   the vtable at VA `0x101c67e0`, slot `+0x40`) is a per-frame update on an RPG
   item/weapon object:
   - `flags |= 0x2` once a saturating counter `+0x212` exceeds the byte threshold
     `+0x210`;
   - `flags &= ~0x2` once `+0x212` exceeds `+0x211` **and** `*(short*)(+0x204) != 0`;
   - both transitions call `FUN_100dcb90(this, 0x1056, 0, -1, -1)` (an anim
     property/event notification).

   So `noviewmodel` is a runtime *output* of an item state machine, not only a
   content `*flags` input.

2. **No direct consumer.** A raw byte scan of every shipped x86 module
   (GameClasses, GameWorld, MXR, MSystem, MCCDyn, RndrGL, `SbzEngine.exe`) found:
   - exactly **one** immediate bit-1 test on `+0x14c` anywhere
     (`0x10037111 test byte [eax+0x14c],2`, ADR-0435 — a *different* object's
     field, inside player-start/camera-entity selection);
   - `test al,2` / `test eax,2` / `and reg,2` / `bt reg,1` within 40 bytes of any
     `+0x14c` read appear **only inside `FUN_100e6aa0` itself**;
   - no `(flags >> 1) & 1` (shift-then-mask / `test reg,1`) form near a `+0x14c`
     read.
   Two earlier byte-scan "hits" (`0x1013F898`, `0x10052C84`) are **false
   positives** (a `JNZ` rel32 operand and the bytes of a `CMP dword [..+0x1a8],0`
   respectively).

## Decision
1. **Approach 1's premise is falsified.** There is no draw gate that tests
   `[item+0x14c] & 2`; the bit has no direct mask reader. Writing `flags |= 2`
   from the hook is therefore *not* expected to suppress the first-person
   viewmodel, and the engine's own `FUN_100e6aa0` toggler would fight such a
   write every frame anyway.
2. **#63 moves to approach 2** — per-draw suppression. This is feasible: the hook
   is an `opengl32.dll` proxy that already **exports** `glBegin`
   (`src/hook/opengl32.def:13`), `glDrawArrays` (`:75`) and `glDrawElements`
   (`:77`) but currently only forwards them; per-draw interception can be added
   there and a viewmodel-pass signature sought (FP projection/camera state, a
   depth-clear boundary, or a distinct model/material binding).
3. Keep ADR-0435 (the flag encoding) — it remains correct and is still useful
   for content cross-checks.

## Consequences
- #63 gets a concrete, unblocked software direction (per-draw interception)
  instead of a runtime engine-state write that the binary does not support.
- The engine-state-write idea is **not** deleted outright: a *copy-based*
  consumer (flags snapshotted into another struct, then compared) cannot be
  excluded by a mask scan; only if approach 2 fails is that traced.

## Not claimed
- That the bit is inert — only that no code path masks it to make a draw
  decision.
- The exact class of the `FUN_100e6aa0` object (layout shows the RPG-item
  `*flags` at `+0x14c`; the MSVC RTTI COL chain is not recoverable from the file).
- What `FUN_100dcb90(..., 0x1056, ...)`, `+0x204`, `+0x210/+0x211/+0x212` mean
  beyond the observed hysteresis behaviour.
- Any in-headset behaviour (nothing here is validated at runtime).

## References
- `docs/rea/reference/GameClasses_NoviewmodelRuntime.txt` (evidence)
- `docs/rea/reference/GameClasses_RPG_ItemFlags.txt`, ADR-0435 (flag encoding)
- GH #63 (duplicate hands), and the new follow-up issue for approach 2
