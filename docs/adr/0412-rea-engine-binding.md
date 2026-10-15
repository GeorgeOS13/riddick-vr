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

## Decision

Keep keystrokes as the sole input path; grow the REA table through the
workflow above.
