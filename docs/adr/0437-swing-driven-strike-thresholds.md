# ADR 0437: Swing-Driven Strike Thresholds Are Shared, INI-Tunable, and Unit-Tested

## Status
**Accepted.** Addresses GH #65 ("Punch/attack gesture does not fire when swinging
the Quest controller") — specifically its *remaining* work ("expose the strike
velocity thresholds in the INI"). Builds on ADR-0403 (guard raise ≠ strike) and
ADR-0434 (shared front-of-HMD envelope), and follows the same extraction pattern
as `combat_guard.h`.

## Context
`e858809` fixed the first-pass bug by dropping the both-trigger
`combatStance.active` gate and the `!inGuardVolume` mask, so a swing is now
driven by controller velocity. But the punch/hook/uppercut classifiers stayed
inline in `UpdateMeleeCounters` (`src/hook/camera_hook.cpp`) with **hard-coded**
thresholds:

```
isForwardPunch  = Vz < -1.20 && handZ < -0.10
isHookPunch     = |Vx| > 1.35 && Vz < -0.15
isUppercutPunch = Vy > 1.35 && handY < -0.10 && Vz < -0.20
dualCrossSlash  = opposing |Vx| > 1.10
```

These numbers decide whether a real swing fires, but could only be changed by
rebuilding the hook. The issue asks for them to be tunable on a headset.

## Decision
Add `src/hook/combat_strike.h` — header-only, dependency-free:

- `StrikeThresholds` (the eight numbers above; field defaults are exactly the
  committed `e858809` values) and `StrikeVelocity` (a 3-vector in m/s).
- `ClassifyStrike(thresholds, leftVel, rightVel, leftY, leftZ, rightY, rightZ)`
  returns `StrikeIntent { dualCrossSlash, forwardPunch, hookPunch,
  uppercutPunch, strike }`, where `strike` is the OR of the four (the combined
  melee-strike gate). Comparisons are strict (`<`/`>`), matching the original
  inline code.

`camera_hook.cpp`:

- `GetStrikeThresholds()` builds the struct from `g_vrConfig` (new `Strike*`
  fields), exactly like `GetGuardEnvelope()` for the guard envelope.
- `UpdateMeleeCounters` now calls `ClassifyStrike(...)` for the four booleans;
  the ADR-0403 comment is kept verbatim: strikes are *not* suppressed inside the
  guard volume, and a guard raise (`Vy>0`, `Vz~0`) still matches no condition.

`vr_config.{h,cpp}`: eight new fields + INI keys, each clamped so a typo cannot
make strikes impossible (e.g. `StrikeForwardVz` clamped to `[-8, -0.20]`,
`StrikeHookVx`/`StrikeUppercutVy`/`StrikeCrossVx` to `[0.10, 8]`).

`tests/combat_strike_tests.cpp` + CTest `combat_strike_thresholds` pin the
defaults, the strict boundaries, per-hand symmetry, the combined gate, the
**ADR-0403 guard-raise regression**, and that tuning the struct changes the
outcome (the INI path).

`verify_reported_fixes.py` gains a #65 invariant (header + shared call site +
`Strike*` config keys + the CTest target).

## Consequences
- Default behaviour is **unchanged** (defaults = the first pass) but every strike
  threshold is now headset-tunable via `riddick_vr.ini`.
- The classifier is shared and unit-tested; the guard context (`inGuardVolume`)
  is retained only as the ADR-0434 shared-envelope source invariant — it no
  longer suppresses strikes.
- `riddick_vr.ini`, `tools/riddick_vr.ini.example` and `src/hook/README.md`
  document the new keys.

## Not claimed
- **Not validated in-headset.** Whether these defaults feel right on a real
  Quest swing (and whether they fire on a legitimate hard swing while never
  firing on a guard raise) is a hardware step; this ADR only makes the numbers
  tunable and pins their semantics.
