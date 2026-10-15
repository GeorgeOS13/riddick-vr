# ADR 0412: REA Engine Binding Workflow (First Step — Scaffold Only)

Status: proposed (scaffold; keystrokes remain the active input path)

## Context

Gesture input currently reaches the Dark Athena engine as emulated
keystrokes (`SendKey`/`SendKeyTap` via `gesture_tap.h`). Reverse-engineering
actual engine action entry points (RndrGL present, player fire/reload/use
dispatch) would remove a layer of latency and fragility, but must be done
incrementally: pin the game version, document candidate signatures, validate
offline, then bind one action at a time behind a config flag.

## Step 1 (this ADR — implemented)

- `config/addresses.json`: version-pinned sigscan table scaffold with
  RndrGL + player-action candidates (`status: candidate-unverified`).
- `tools/validate_addresses.py`: schema + `base+offset` self-test that logs
  resolution math without attaching to any process.
- Explicit non-goal: **do NOT replace keystrokes yet.** Nothing in the hook
  reads this table.

## Documented REA workflow (one action at a time)

1. Pin the target binary: record `DarkAthena.exe` version + module hashes in
   `game_version_pin` before any pattern work.
2. Locate ONE candidate (e.g. `Player.FireAction`) in a disassembler;
   record byte pattern + offset with `status: candidate-unverified`.
3. Run `python3 tools/validate_addresses.py` — schema/self-test must PASS.
4. Verify live in a debugger session (breakpoint on resolved
   `base+offset`, confirm hit on the intended in-game action only).
5. Flip status to `verified-live`, then bind behind an INI flag
   (default OFF) with keystroke fallback intact.
6. Repeat for the next action. Never bind more than one unverified entry.

## Follow-ups

- Live verification sessions per entry (requires game + debugger).
- INI-gated engine-call path with automatic keystroke fallback.
- Pattern re-pinning procedure on game patches.

## Static recon 2026-10-15 (GH #34, branch `docs/rea-msystem`, read-only)

`dumpbin 14.42.34436.0 /EXPORTS` on `MSystem.dll`
(timestamp 2009-03-25, 5.1MB): **2701 exports**. 145 match
`action/input/player/use/fire/reload/trigger/weapon/interact`, but every one
is `CInputContext*` / `CInputEnvelope*` / `CScanKey*` polling plumbing
(`IsPressed`, `IsNotPressed`, `KeyPressed`, `DownKey`/`UpKey`,
`QueueScanKey`, `GetMouseButtons`/`GetMousePosition`, …) or the sound-mixer
`Con_Snd_Mix_AutoReload`/`Reload`. **Zero exports for
Trigger/Action/Weapon/Fire/Player/Interact dispatch** — MSystem is the
input-polling abstraction, not the action dispatcher.

Printable-runs (`>=5` chars) scan for `TRIGGER REFILL usethirdperson slot
ammo shiv NanoMED Eyeshine`: **0 hits in both `DarkAthena.exe` (23,477
strings) and `MSystem.dll` (19,381 strings)**. `MXR.dll` (4.6MB) is the only
module with a literal `TRIGGER` token (+ `movement-slots`/`rotation-slots`,
`XREngine_DepthReload`); its 6 exports are just `DllMain`/MRTC container/
`Alloc_SkeletonInst`, so dispatch is internal static code. `DarkAthena.exe`
imports confirm the thin client: `USER32/GDI32/WINMM/WS2_32 +
MSystem/MCCDyn/dvm`, no `dinput`/DirectInput.

### Ranked candidate table (next REA/Ghidra step each)

| # | Export / token | Module | Why it matters | Next REA/Ghidra step |
|---|---|---|---|---|
| 1 | literal `TRIGGER` string | `MXR.dll` | Only TRIGGER token in any shipped module; XR runtime is the plausible action owner | x-ref `TRIGGER` in Ghidra, find consuming function = action dispatch candidate #1 |
| 2 | `CInputContextCore::IsPressed` (RVA `00156B40`), `IsNotPressed` (`00156B70`), `KeyPressed` (`001575E0`) | `MSystem.dll` | Per-frame polled state the engine's action layer must read; confirmed exported polling API | Breakpoint + back-trace callers to the per-frame action consumer; fallback synthetic-input point |
| 3 | `QueueScanKey` (`00159240`), `DownKey` (`001572D0`) / `UpKey` (`001574A0`), `AddScanKey` (`00157250`) | `MSystem.dll` | Injection end of the same queue SendInput keystrokes travel; lowest-risk engine-call seam | Ghidra: signature + queue struct layout; verify with synthetic scancode in debugger |
| 4 | `Con_Snd_Mix_AutoReload` / `Con_Snd_Mix_Reload` | `MSystem.dll` | Only `Reload`-named exports; sound-mixer, almost certainly NOT gameplay reload — recorded to kill the false positive | Confirm no gameplay caller; exclude from action table |
| 5 | `MCCDyn.dll` (1.2MB, imported by exe, 0 action strings) | `MCCDyn.dll` | Only non-scanned game-logic candidate left in the import closure | Ghidra import/function survey for action/config dispatch |
| 6 | gameplay tokens in content archives | content (non-DLL) | Tokens absent from ALL binaries ⇒ likely live in packed config/script archives | Locate + scan game content packs for `TRIGGER/REFILL/usethirdperson/slot/ammo/shiv/NanoMED/Eyeshine` |

Recorded in `config/addresses.json` under `research_notes`
(validated by `tools/validate_addresses.py`); `Player.*` entries stay
`candidate-unverified`, keystrokes remain the sole input path.

## Decision

Keep keystrokes as the sole input path; grow the REA table through the
workflow above.
