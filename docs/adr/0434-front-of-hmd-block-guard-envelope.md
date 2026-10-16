# ADR 0434: Front-of-HMD Block Guard Envelope (Shared, INI-Tunable)

## Status
**Accepted.** Implements GH #62 ("Physical block/attack: guard = hands in front
of the HMD, swing = attack (not below the view)").

## Context
GH #62 is a player-visible comfort/combat bug. Two behaviours had to change:

1. **Block = hands in front of the HMD.** The guard must trigger on a hand
   *pose* with the fists raised in front of the face — never on a low rest
   hold — and the numbers must be tunable on a headset.
2. **Attack = controller swing.** A fast forward/hook/uppercut swing must always
   fire the strike; raising the hands into guard must never be read as an
   attack.

The prior code had **two different hand volumes**:

- The guard *pose* detector (added in `05ae8a6`) used a tight, front-of-face
  window: `y ∈ [-0.16, 0.32]`, `z ∈ [-0.60, -0.12]`, span `< 0.50`
  (`src/hook/camera_hook.cpp`).
- The strike gate still used the **old, loose** volume: `y ∈ [-0.42, 0.32]`,
  `z ∈ [-0.68, -0.05]`, span `< 0.56`, and `isMeleeStrike` was masked by
  `&& !inGuardVolume`.

The old `y` floor of `-0.42 m` sits **below the view**, and its `z` ceiling of
`-0.05 m` reaches **inside** the view, so a forward/hook swing thrown from a
raised guard landed inside the loose volume and was suppressed by
`!inGuardVolume` — exactly the "raising the hands to guard can mask or cancel
attacks" symptom in the issue.

## Decision
Extract the front-of-HMD window into a small, dependency-free, unit-tested
header, `src/hook/combat_guard.h`:

```cpp
struct GuardEnvelope {
    float yMin = -0.16f;   // no lower than just under the chin (never below the view)
    float yMax =  0.32f;
    float zMin = -0.60f;   // farthest forward the guard may be held
    float zMax = -0.12f;   // nearest to the face
    float maxHandSpan = 0.50f;
};
bool GuardHandInWindow(const GuardEnvelope&, float y, float z);
bool InGuardEnvelope(const GuardEnvelope&, float lY, float lZ,
                     float rY, float rZ, float handSpanX);
```

Head-local coordinates: `+X` right, `+Y` up, `-Z` forward into the view.

Both call sites now share the envelope:

1. **Guard pose** (`camera_hook.cpp`, `guardPoseDetected`): `chinLevel`,
   `inFrontOfFace` and `handsClose` are derived from the envelope fields instead
   of inline literals. The **defaults reproduce the previous pose window
   exactly**, so the pose behaviour is unchanged; only the values are now
   tunable.
2. **Strike gate** (`UpdateMeleeCounters`): `inGuardVolume` is now
   `InGuardEnvelope(guardVolume, …)` — the same tight front-of-HMD window. A
   genuine fast swing that leaves the strict chin-to-forehead window (lower,
   closer, or wider) now registers; slowly raising the hands into the window
   still carries no punch velocity, so it is still never read as an attack.

The envelope is sourced from `g_vrConfig` (`GetGuardEnvelope()`) and exposed as
INI keys `GuardYMin/GuardYMax/GuardZMin/GuardZMax/GuardMaxHandSpan` (clamped;
an inverted window is rejected so a typo can never make the guard unreachable).

## Consequences
- GH #62 requirement 1 ("front of the view, not below") holds for **both** the
  pose and the strike gate; the low-hold floor moved from `-0.42 m` to `-0.16 m`.
- GH #62 requirement 2 ("a fast swing always fires") holds: the masking volume
  no longer overlaps the ordinary throw arc.
- GH #62 requirement 4 (INI knobs) is satisfied for the guard envelope; the
  swing velocity thresholds themselves are unchanged (still the ADR-0403 values).
- New CTest `combat_guard_envelope` (`tests/combat_guard_tests.cpp`) pins the
  window semantics, the boundary inclusivity, the low-hold rejection (the #62
  regression), and the sign-agnostic span.

## Not claimed
- Not validated in-headset: the defaults are derived from the shipped ADR-0400
  window and must be tuned on hardware (`Guard*` keys). The issue's acceptance
  ("only the VR/native hands visible … validated in-headset") is a later step.
- The holster/equip suppression predicate (`inGuardPose` at the holster gate,
  `y > -0.38`) is **left as-is**: it only suppresses slot toggles near a raised
  hand, and tightening it would enable holster gestures in more positions.
- The swing *velocity* thresholds (`1.20/1.35 m/s`) are unchanged.
- This does not implement the physical block/attack **haptics** beyond the
  existing per-gesture calls.
