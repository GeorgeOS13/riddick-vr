#!/usr/bin/env python3
"""Read-only decoder for Starbreeze ``.xsa`` animation sets.

The retail Riddick character/weapon animation clips are ``MOS DATAFILE2.0``
containers (the same container as ``.xmd`` models; see ``tools/mos_xmd.py`` and
``docs/adr/0414-mos-datafile2-loader-container.md``) whose first child node is
named ``COMPRESSEDANIMATIONSET2``.  This module decodes that node's payload so
the per-bone **rotations/translations** the native first-person hand/arm draw
path needs (GH #46) can be sourced offline instead of from the running game
(GH #58).

Layout recovered by decompiling the shipped ``MXR.dll`` loader (EFBB 2004,
x86) with Ghidra 12.1.4 via ``rea`` (see
``docs/adr/0422-xsa-compressed-animation-set.md`` for the full evidence):

  * dispatcher ``FUN_1001d7c0`` probes ``ANIMATIONSET`` / ``ANIMATIONTRACKSET``
    / ``COMPRESSEDANIMATIONSET`` / ``COMPRESSEDANIMATIONSET2`` and for the last
    allocates a ``CXR_Anim_Compressed`` (vtable ``0x101442b8``) and calls its
    slot ``+0x20``.
  * that slot is ``FUN_10076d20`` (``XRAnimCompressed.cpp``): for each chunk it
    reads the node's ``count`` elements, each of a fixed element size, straight
    out of the file via ``CCFile::Read``.

  COMPRESSEDANIMATIONSET2 children (declared node ``count`` == element count,
  ``data_size`` == element size * count):

      QUATERNIONS        count * 4 x i16          CXR_Anim_Quatint16
                         (x, y, z, w) / 32767 -> tuning: unit quaternion
      MOVES              count * 3 x f32          translation keys
      TRACKINDICES       count * u16
      QUATERNIONINDICES  count * u16              per-key-slot index pool
      KEYFRAMETIMES      count * f32              keyframe times (seconds)
      TRACKS             count * { u16 start; u16 flags }
                         (CXR_Anim_CompressedTrack; flags bit 0x0001 =
                         movement keys, 0x8000 = single/static key)
      DATAKEYS           variable-length records  (CXR_Anim_DataKey; opaque here)
      SEQUENCES          variable-length records  (CXR_Anim_Sequence, v0x103)

  SEQUENCE record (version ``0x103``), from ``CXR_Anim::ReadCompressed``
  ``FUN_1001a5e0`` (base read) + ``CXR_Anim_Sequence::Read`` ``FUN_100774f0``
  (which reads the eight tail words straight into the object at ``+0x20``)::

      u32  version            = 0x103
      u16  field_a
      u16  field_b
      str  name               (u32 length + bytes, padded to 4)
      str  name2              (u32 length + bytes, padded to 4)
      u16  version2           = 0x100
      u16  track_start        (+0x20)  first TRACKINDICES slot of this sequence
      u16  track_mid          (+0x22)  always track_start + rotation_count
      u8   rotation_count     (+0x24)
      u8   movement_count     (+0x25)
      u16  time_start         (+0x26)  first KEYFRAMETIMES slot
      u16  time_count         (+0x28)
      u16  extra_a            (+0x2a)
      u16  extra_c            (+0x2c)

  Each sequence therefore owns two contiguous windows: its **key window**
  ``[track_start, track_start + rotation_count + movement_count)`` into
  ``TRACKINDICES`` and its **time window** ``[time_start, time_start +
  time_count)`` into ``KEYFRAMETIMES``.  Over every one of the **313** shipped
  animation sets the non-empty windows tile their pools **exactly once**
  (enforced by :func:`window_tiling_problems` from :meth:`AnimSet.validate`) and
  ``track_mid == track_start + rotation_count`` holds for every sequence.

**Clip -> bone (slot) binding (pinned from the runtime sampler):**

  ``TRACKINDICES`` values are indices into ``TRACKS`` (``max < TRACKS.count``
  over the whole corpus).  A clip's key window is a **slot -> track** table, not
  a set of tracks: its first ``rotation_count`` entries are indexed by the
  skeleton **rotation slot** and its next ``movement_count`` entries by the
  **movement slot** (``CXR_Skeleton`` caps those at ``0x80`` / ``0x20``;
  ``.xmd`` ``NODES`` carry the ``rotation_slot`` / ``movement_slot`` a bone
  listens on -- ``tools/mos_xmd.py``, ADR-0418).  The per-slot sampler is
  ``FUN_100776b0``, reached from ``FUN_10077f20`` (the sequence object's
  ``+0x24`` method): for slot ``j`` it reads
  ``track = TRACKINDICES[window_start + j]`` and
  ``(start, flags) = TRACKS[track]``.

  *Key count.*  ``flags & 0x8000`` (``TRACK_FLAG_SINGLE``) means **one** key;
  otherwise the track owns exactly ``sequence.time_count`` keys.  Keys live in
  ``QUATERNIONINDICES`` at ``[start, start + key_count)``.

  *Pool.*  A rotation-window slot always indexes ``QUATERNIONS``
  (``CXR_Anim_Quatint16``, 4 x i16 / 32767).  A movement-window slot indexes
  ``MOVES`` (3 x f32) **unless** the track's ``flags & 0x0001``
  (``TRACK_FLAG_MOVEMENT``) is set, in which case it indexes ``QUATERNIONS``
  too.  The runtime sampler (above) reads the value at
  ``QUATERNIONINDICES[start + key]`` and dereferences it in the pool selected by
  the *window*; the flag override is what the 81 shipped movement blocks that
  exceed ``MOVES.count`` (e.g. ``Johns.xsa`` track 895, values 9339.. up to
  11229 with ``MOVES.count == 7170``) prove: every one fits ``QUATERNIONS`` and
  none exceeds it.

  ``flags & 0x4000`` (never set in shipped data) yields the identity/zero key.
  ``KEYFRAMETIMES`` is indexed directly by the clip's ``[time_start,
  time_start + time_count)`` window -- no indirection.

The parser is dependency-free and strictly read-only: it never writes next to
the game install and never launches the game.

Usage:
    python tools/xsa_anim.py <file.xsa> [--json] [--tracks] [--sequences] [--tree]
    python tools/xsa_anim.py <file.xsa> --quaternions N
    python tools/xsa_anim.py <file.xsa> --clip NAME
    python tools/xsa_anim.py --inventory <dir> [--json]
    python tools/xsa_anim.py <file.xsa> --check
    python tools/xsa_anim.py --inventory <dir> --check
"""

from __future__ import annotations

import argparse
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mos_xmd  # noqa: E402  (path injected above)
from mos_xmd import Xmd, XmdError  # noqa: E402

ANIMSET = "COMPRESSEDANIMATIONSET2"

# Fixed element sizes, proven from MXR.dll ``FUN_10076d20``: for every shipped
# ``COMPRESSEDANIMATIONSET2`` file ``data_size == element_size * node.count``.
ELEMENT_SIZES = {
    "QUATERNIONS": 8,
    "MOVES": 12,
    "TRACKINDICES": 2,
    "QUATERNIONINDICES": 2,
    "KEYFRAMETIMES": 4,
    "TRACKS": 4,
}
REQUIRED_CHUNKS = ("DIRECTORYHEADER", "QUATERNIONS", "MOVES", "TRACKINDICES",
                   "QUATERNIONINDICES", "KEYFRAMETIMES", "TRACKS", "DATAKEYS",
                   "SEQUENCES")

# CXR_Anim_Quatint16: four signed 16-bit components scaled by 1 / 32767.
QUAT_SCALE = 1.0 / 32767.0
QUAT_NORM_TOLERANCE = 1e-3

# CXR_Anim_CompressedTrack flags (proven set across all shipped tracks).
# 0x8000 SINGLE: the track owns exactly one key (not ``time_count``).
# 0x0001 MOVEMENT: on a *movement-window* slot this bit routes the key values to
#   the QUATERNIONS pool instead of MOVES (corpus-proven; see module docstring).
TRACK_FLAG_MOVEMENT = 0x0001
TRACK_FLAG_SINGLE = 0x8000
KNOWN_TRACK_FLAGS = TRACK_FLAG_MOVEMENT | TRACK_FLAG_SINGLE

# The two halves of a clip's key window, indexed by skeleton slot number.
ROTATION_WINDOW = "rotation"
MOVEMENT_WINDOW = "movement"
WINDOWS = (ROTATION_WINDOW, MOVEMENT_WINDOW)

# The value pools a QUATERNIONINDICES entry can dereference to.
POOL_QUATERNIONS = "quaternions"
POOL_MOVES = "moves"

# CXR_Anim_Sequence record.
SEQ_VERSION = 0x103
SEQ_TAIL_VERSION = 0x100
SEQ_ALIGN = 4


class XsaError(Exception):
    pass


class Track:
    """One ``CXR_Anim_CompressedTrack`` (4 bytes)."""

    __slots__ = ("start", "flags")

    def __init__(self, start: int, flags: int):
        self.start = start
        self.flags = flags

    @property
    def has_movement(self) -> bool:
        return bool(self.flags & TRACK_FLAG_MOVEMENT)

    @property
    def is_single(self) -> bool:
        return bool(self.flags & TRACK_FLAG_SINGLE)

    def as_dict(self):
        return {"start": self.start, "flags": self.flags,
                "movement": self.has_movement, "single": self.is_single}


class Sequence:
    """One ``CXR_Anim_Sequence`` record (version 0x103)."""

    __slots__ = ("version", "field_a", "field_b", "name", "name2",
                 "version2", "fields")

    def __init__(self, version, field_a, field_b, name, name2, version2,
                 fields):
        self.version = version
        self.field_a = field_a
        self.field_b = field_b
        self.name = name
        self.name2 = name2
        self.version2 = version2
        self.fields = fields

    def as_dict(self):
        return {"version": self.version, "field_a": self.field_a,
                "field_b": self.field_b, "name": self.name, "name2": self.name2,
                "version2": self.version2, "fields": list(self.fields),
                "track_start": self.track_start, "track_mid": self.track_mid,
                "rotation_count": self.rotation_count,
                "movement_count": self.movement_count,
                "time_start": self.time_start, "time_count": self.time_count,
                "key_window": list(self.key_window),
                "time_window": list(self.time_window)}

    # -- sequence-table fields (CXR_Anim_Sequence::Read, FUN_100774f0) -----
    @property
    def track_start(self) -> int:
        """First ``TRACKINDICES`` slot owned by this sequence."""
        return self.fields[0]

    @property
    def track_mid(self) -> int:
        """End of the rotation part == ``track_start + rotation_count``."""
        return self.fields[1]

    @property
    def rotation_count(self) -> int:
        """Rotation entries in this sequence's key window."""
        return self.fields[2]

    @property
    def movement_count(self) -> int:
        """Movement entries in this sequence's key window."""
        return self.fields[3]

    @property
    def time_start(self) -> int:
        """First ``KEYFRAMETIMES`` slot used by this sequence."""
        return self.fields[4]

    @property
    def time_count(self) -> int:
        return self.fields[5]

    @property
    def key_window(self) -> tuple:
        """``(lo, hi)`` into ``TRACKINDICES`` (empty windows are allowed)."""
        return (self.track_start,
                self.track_start + self.rotation_count + self.movement_count)

    @property
    def time_window(self) -> tuple:
        """``(lo, hi)`` into ``KEYFRAMETIMES`` (empty windows are allowed)."""
        return (self.time_start, self.time_start + self.time_count)


class SlotBinding:
    """One clip slot -> ``TRACKS`` binding, resolved through the pools.

    Produced by :meth:`AnimSet.slot_binding`.  ``pool`` is the pool the
    ``pool_indices`` dereference to (:data:`POOL_QUATERNIONS` or
    :data:`POOL_MOVES`); ``start``/``count`` locate the key block inside
    ``QUATERNIONINDICES``.
    """

    __slots__ = ("window", "slot", "track", "flags", "start", "count", "pool",
                 "pool_indices")

    def __init__(self, window, slot, track, flags, start, count, pool,
                 pool_indices):
        self.window = window
        self.slot = slot
        self.track = track
        self.flags = flags
        self.start = start
        self.count = count
        self.pool = pool
        self.pool_indices = pool_indices

    @property
    def single(self) -> bool:
        return bool(self.flags & TRACK_FLAG_SINGLE)

    @property
    def is_rotation(self) -> bool:
        return self.pool == POOL_QUATERNIONS

    def as_dict(self):
        return {"window": self.window, "slot": self.slot, "track": self.track,
                "flags": self.flags, "start": self.start, "count": self.count,
                "pool": self.pool, "pool_indices": list(self.pool_indices)}


def _u16(b: bytes, off: int) -> int:
    return struct.unpack_from("<H", b, off)[0]


def _u32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


def _align(off: int, align: int) -> int:
    return (off + align - 1) // align * align


def _bracket(times, t: float):
    """Return ``(i, frac)`` bracketing ``t`` in the sorted ``times`` list."""
    n = len(times)
    if n < 2:
        return 0, 0.0
    if t <= times[0]:
        return 0, 0.0
    if t >= times[-1]:
        return n - 2, 1.0
    for i in range(1, n):
        if t <= times[i]:
            t0, t1 = times[i - 1], times[i]
            frac = 0.0 if t1 == t0 else (t - t0) / (t1 - t0)
            return i - 1, frac
    return n - 2, 1.0


def _lerp(a, b, f):
    return tuple(x + (y - x) * f for x, y in zip(a, b))


def _lerp_series(times, values, t):
    i, f = _bracket(times, t)
    return _lerp(values[i], values[i + 1], f)


def _nlerp(a, b, f):
    """Normalised lerp of two float quaternions (x, y, z, w)."""
    dot = sum(x * y for x, y in zip(a, b))
    if dot < 0.0:
        b = tuple(-y for y in b)
    q = tuple(x + (y - x) * f for x, y in zip(a, b))
    n = sum(c * c for c in q) ** 0.5
    if n == 0.0:
        return (0.0, 0.0, 0.0, 1.0)
    return tuple(c / n for c in q)


def _nlerp_series(times, quats, t):
    i, f = _bracket(times, t)
    return _nlerp(quats[i], quats[i + 1], f)


def window_tiling_problems(name: str, windows, total: int) -> list:
    """Validate that ``windows`` tile ``[0, total)`` exactly once per slot.

    Empty windows (``lo == hi``) are ignored, matching the shipped corpus where
    several sequences carry a zero-length key/time window. Returns a list of
    human-readable problems (empty == the windows are an exact partition).
    """
    problems = []
    cover = [0] * total
    for lo, hi in windows:
        if lo < 0 or hi > total or hi < lo:
            return [f"{name} window {lo}..{hi} out of range (count {total})"]
        for k in range(lo, hi):
            cover[k] += 1
    gaps = [k for k, c in enumerate(cover) if c == 0]
    dup = [k for k, c in enumerate(cover) if c > 1]
    if gaps:
        problems.append(f"{name} tiling leaves {len(gaps)} uncovered slot(s) "
                        f"(first {gaps[0]})")
    if dup:
        problems.append(f"{name} tiling covers {len(dup)} slot(s) twice "
                        f"(first {dup[0]})")
    return problems


def decode_quaternions(b: bytes, off: int, count: int):
    """``count`` ``CXR_Anim_Quatint16`` (4 x i16 / 32767)."""
    return [struct.unpack_from("<4h", b, off + i * 8) for i in range(count)]


def decode_moves(b: bytes, off: int, count: int):
    """``count`` translation keys (3 x f32, model units)."""
    return [struct.unpack_from("<3f", b, off + i * 12) for i in range(count)]


def decode_u16_array(b: bytes, off: int, count: int):
    return list(struct.unpack_from("<%dH" % count, b, off))


def decode_f32_array(b: bytes, off: int, count: int):
    return list(struct.unpack_from("<%df" % count, b, off))


def decode_tracks(b: bytes, off: int, count: int):
    return [Track(*struct.unpack_from("<HH", b, off + i * 4))
            for i in range(count)]


def _read_string(b: bytes, off: int, end: int):
    if off + 4 > end:
        raise XsaError(f"sequence string length overruns chunk at 0x{off:X}")
    n = _u32(b, off)
    off += 4
    if n > end - off:
        raise XsaError(f"sequence string length {n} overruns chunk at 0x{off:X}")
    s = b[off:off + n].decode("latin1")
    off = _align(off + n, SEQ_ALIGN)
    return s, off


def decode_sequences(b: bytes, count: int):
    """Decode ``count`` ``CXR_Anim_Sequence`` records. Returns (list, consumed).

    Only version ``0x103`` is shipped (verified across every ``.xsa``); the
    other versions exist in ``FUN_1001a5e0`` but no retail asset selects them.
    """
    end = len(b)
    out = []
    off = 0
    for _ in range(count):
        if off + 4 > end:
            raise XsaError("sequence record overruns chunk")
        version = _u32(b, off)
        off += 4
        if version != SEQ_VERSION:
            raise XsaError(f"unsupported sequence version 0x{version:X} "
                           f"(only 0x{SEQ_VERSION:X} is shipped)")
        field_a = _u16(b, off)
        field_b = _u16(b, off + 2)
        off += 4
        name, off = _read_string(b, off, end)
        name2, off = _read_string(b, off, end)
        if off + 2 > end:
            raise XsaError("sequence version2 overruns chunk")
        version2 = _u16(b, off)
        off += 2
        if version2 != SEQ_TAIL_VERSION:
            raise XsaError(f"unexpected sequence version2 0x{version2:X}")
        if off + 14 > end:
            raise XsaError("sequence tail overruns chunk")
        fields = struct.unpack_from("<HHBBHHHH", b, off)
        off += 14
        out.append(Sequence(version, field_a, field_b, name, name2, version2,
                            fields))
    return out, off


class AnimSet:
    """A decoded ``COMPRESSEDANIMATIONSET2`` (or a non-anim ``MOS`` file)."""

    def __init__(self, path: str):
        self.path = path
        self.xmd = Xmd(path)
        self.nodes = {n.name: n for n in self.xmd.nodes}
        self.has_anim = ANIMSET in self.nodes
        self.quaternions = []
        self.moves = []
        self.track_indices = []
        self.quaternion_indices = []
        self.keyframe_times = []
        self.tracks = []
        self.sequences = []
        self.sequences_consumed = 0
        if self.has_anim:
            self._decode()

    def _payload(self, name: str):
        n = self.nodes.get(name)
        if not n or not n.data_size:
            return b"", 0
        return self.xmd.data[n.data_offset:n.data_offset + n.data_size], n.count

    def _decode(self):
        b, n = self._payload("QUATERNIONS")
        self.quaternions = decode_quaternions(b, 0, n)
        b, n = self._payload("MOVES")
        self.moves = decode_moves(b, 0, n)
        b, n = self._payload("TRACKINDICES")
        self.track_indices = decode_u16_array(b, 0, n)
        b, n = self._payload("QUATERNIONINDICES")
        self.quaternion_indices = decode_u16_array(b, 0, n)
        b, n = self._payload("KEYFRAMETIMES")
        self.keyframe_times = decode_f32_array(b, 0, n)
        b, n = self._payload("TRACKS")
        self.tracks = decode_tracks(b, 0, n)
        b, n = self._payload("SEQUENCES")
        if n:
            self.sequences, self.sequences_consumed = decode_sequences(b, n)

    # -- reporting -------------------------------------------------------
    def quaternion(self, index: int):
        """The ``index``-th pooled rotation as a float 4-tuple."""
        q = self.quaternions[index]
        return tuple(c * QUAT_SCALE for c in q)

    def sequence_names(self):
        return [s.name for s in self.sequences if s.name]

    def sequence_by_name(self, name: str):
        """First sequence with this clip name (name2 fallback), or ``None``."""
        for s in self.sequences:
            if s.name == name or s.name2 == name:
                return s
        return None

    # -- clip -> slot -> track -> pool (FUN_100776b0) ---------------------
    def window_tracks(self, seq, window) -> list:
        """The track ordinals of a clip's ``rotation``/``movement`` window."""
        kw = seq.key_window
        lo, hi = ((kw[0], seq.track_mid) if window == ROTATION_WINDOW
                  else (seq.track_mid, kw[1]))
        return self.track_indices[lo:hi]

    def window_slot_track(self, seq, window, slot: int) -> int:
        """The ``TRACKS`` ordinal a clip's ``window`` slot ``j`` listens on."""
        return self.window_tracks(seq, window)[slot]

    def movement_pool(self, ordinal: int) -> str:
        """Pool a movement-window track's values dereference to."""
        if self.tracks[ordinal].flags & TRACK_FLAG_MOVEMENT:
            return POOL_QUATERNIONS
        return POOL_MOVES

    def pool_for(self, window: str, ordinal: int) -> str:
        """Pool a ``window`` slot's key values dereference to."""
        if window == ROTATION_WINDOW:
            return POOL_QUATERNIONS
        return self.movement_pool(ordinal)

    def slot_binding(self, seq, window: str, slot: int) -> "SlotBinding":
        """Resolve ``(clip, window, slot)`` to a :class:`SlotBinding`."""
        tracks = self.window_tracks(seq, window)
        if slot < 0 or slot >= len(tracks):
            raise IndexError(f"{window} slot {slot} out of range 0..{len(tracks)}")
        ordinal = tracks[slot]
        tr = self.tracks[ordinal]
        pool = self.pool_for(window, ordinal)
        count = 1 if tr.is_single else seq.time_count
        block = self.quaternion_indices[tr.start:tr.start + max(count, 0)]
        return SlotBinding(window, slot, ordinal, tr.flags, tr.start, count,
                           pool, block)

    def slot_key_times(self, seq) -> list:
        """The clip's ``KEYFRAMETIMES`` window (one time per multi-key track)."""
        lo, hi = seq.time_window
        return self.keyframe_times[lo:hi]

    def slot_bindings(self, seq, window: str) -> list:
        """All :class:`SlotBinding`\\ s of a clip's ``window``, in slot order."""
        return [self.slot_binding(seq, window, j)
                for j in range(len(self.window_tracks(seq, window)))]

    def clip_bindings(self, seq) -> dict:
        """Reproducible ``clip -> slot -> track -> pool`` record for one clip."""
        d = {"name": seq.name, "rotation_count": seq.rotation_count,
             "movement_count": seq.movement_count, "time_count": seq.time_count,
             "key_window": list(seq.key_window),
             "time_window": list(seq.time_window)}
        for window in WINDOWS:
            d[window] = [b.as_dict() for b in self.slot_bindings(seq, window)]
        return d

    def slot_rotation_keys(self, seq, slot: int) -> list:
        """The rotation keys a clip's ``slot`` drives, as float quaternions."""
        b = self.slot_binding(seq, ROTATION_WINDOW, slot)
        return [self.quaternion(i) for i in b.pool_indices]

    def slot_move_keys(self, seq, slot: int) -> list:
        """The translation keys a clip's movement ``slot`` drives (3-tuples)."""
        b = self.slot_binding(seq, MOVEMENT_WINDOW, slot)
        if b.pool != POOL_MOVES:
            raise ValueError(f"movement slot {slot} carries QUATERNIONS keys")
        return [self.moves[i] for i in b.pool_indices]

    def sample_rotation(self, seq, slot: int, t: float):
        """Sample a rotation slot at time ``t`` (nlerp between keyframes).

        The shipping engine interpolates with a Hermite spline
        (``FUN_100776b0``); this is the offline normalised-lerp approximation.
        """
        b = self.slot_binding(seq, ROTATION_WINDOW, slot)
        quats = [self.quaternion(i) for i in b.pool_indices]
        if not quats:
            return (0.0, 0.0, 0.0, 1.0)
        if len(quats) == 1:
            return quats[0]
        return _nlerp_series(self.slot_key_times(seq), quats, t)

    def sample_move(self, seq, slot: int, t: float):
        """Sample a translation movement slot at ``t`` (linear interpolation)."""
        b = self.slot_binding(seq, MOVEMENT_WINDOW, slot)
        if b.pool != POOL_MOVES:
            raise ValueError(f"movement slot {slot} carries QUATERNIONS keys")
        moves = [self.moves[i] for i in b.pool_indices]
        if not moves:
            return (0.0, 0.0, 0.0)
        if len(moves) == 1:
            return moves[0]
        return _lerp_series(self.slot_key_times(seq), moves, t)

    def summary(self) -> dict:
        return {
            "has_animation_set": self.has_anim,
            "quaternions": len(self.quaternions),
            "moves": len(self.moves),
            "track_indices": len(self.track_indices),
            "quaternion_indices": len(self.quaternion_indices),
            "keyframe_times": len(self.keyframe_times),
            "tracks": len(self.tracks),
            "sequences": len(self.sequences),
            "sequence_names": self.sequence_names(),
            "duration": (max(self.keyframe_times)
                         if self.keyframe_times else 0.0),
        }

    def as_dict(self, with_tracks: bool = False):
        d = {"path": self.path, "summary": self.summary(),
             "problems": self.validate()}
        if with_tracks:
            d["tracks"] = [t.as_dict() for t in self.tracks]
            d["sequences"] = [s.as_dict() for s in self.sequences]
        return d

    # -- validation ------------------------------------------------------
    def validate(self) -> list[str]:
        """Structural self-check. Returns a list of problems (empty == ok).

        Confirms the shipped-loader invariants: the six fixed-size chunks tile
        their node payloads exactly, every pooled quaternion is unit-length, the
        track flag set is exactly {0, movement, single}, every track ``start``
        is a valid ``QUATERNIONINDICES`` slot, and the sequence records consume
        the ``SEQUENCES`` payload exactly.
        """
        problems: list[str] = []
        if not self.has_anim:
            return problems
        for name in REQUIRED_CHUNKS:
            if name not in self.nodes:
                problems.append(f"missing {name} chunk")
        for name, size in ELEMENT_SIZES.items():
            n = self.nodes.get(name)
            if n is None:
                continue
            if n.data_size != size * n.count:
                problems.append(
                    f"{name} payload {n.data_size} != {size} * count "
                    f"{n.count}")
        badq = 0
        worst = 0.0
        for q in self.quaternions:
            norm = sum(c * c for c in q) ** 0.5 * QUAT_SCALE
            if abs(norm - 1.0) > QUAT_NORM_TOLERANCE:
                badq += 1
                worst = max(worst, abs(norm - 1.0))
        if badq:
            problems.append(f"{badq} quaternions are not unit length "
                            f"(worst |norm-1| {worst:.5f})")
        qi_count = len(self.quaternion_indices)
        for i, t in enumerate(self.tracks):
            if t.flags & ~KNOWN_TRACK_FLAGS:
                problems.append(f"track {i} has unknown flags 0x{t.flags:X}")
            if qi_count and t.start >= qi_count:
                problems.append(f"track {i} start {t.start} >= "
                                f"QUATERNIONINDICES count {qi_count}")
        seq = self.nodes.get("SEQUENCES")
        if seq is not None and seq.count:
            if self.sequences_consumed != seq.data_size:
                problems.append(
                    f"SEQUENCES tiling {self.sequences_consumed} != "
                    f"{seq.data_size}")
        elif seq is not None and seq.data_size:
            problems.append(f"SEQUENCES has {seq.data_size} bytes but count 0")

        # Sequence table (CXR_Anim_Sequence::Read, FUN_100774f0): each sequence
        # owns a contiguous TRACKINDICES key window [track_start,
        # track_start + rotation_count + movement_count) and a contiguous
        # KEYFRAMETIMES time window [time_start, time_start + time_count); the
        # non-empty windows tile their pools exactly (proven over all 313
        # shipped animation sets). TRACKINDICES values index TRACKS.
        for i, s in enumerate(self.sequences):
            if s.track_mid != s.track_start + s.rotation_count:
                problems.append(
                    f"sequence {i} track_mid {s.track_mid} != track_start "
                    f"{s.track_start} + rotation_count {s.rotation_count}")
        problems.extend(window_tiling_problems(
            "TRACKINDICES", [s.key_window for s in self.sequences],
            len(self.track_indices)))
        problems.extend(window_tiling_problems(
            "KEYFRAMETIMES", [s.time_window for s in self.sequences],
            len(self.keyframe_times)))
        if self.tracks and self.track_indices:
            mti = max(self.track_indices)
            if mti >= len(self.tracks):
                problems.append(f"TRACKINDICES max {mti} >= TRACKS count "
                                f"{len(self.tracks)}")

        # Clip slot -> track -> key block -> pool (FUN_100776b0).  Every
        # rotation-window value must fit QUATERNIONS; every movement-window
        # value must fit MOVES unless the track's 0x0001 flag routes it to
        # QUATERNIONS.  This holds over the shipped corpus with 0 violations.
        for si, s in enumerate(self.sequences):
            for window in WINDOWS:
                for slot, ordinal in enumerate(self.window_tracks(s, window)):
                    if ordinal >= len(self.tracks):
                        continue  # already reported above
                    tr = self.tracks[ordinal]
                    count = 1 if tr.is_single else s.time_count
                    if count <= 0:
                        continue
                    end = tr.start + count
                    if end > qi_count:
                        problems.append(
                            f"sequence {si} {window} slot {slot} track "
                            f"{ordinal} key block {tr.start}..{end} overruns "
                            f"QUATERNIONINDICES count {qi_count}")
                        continue
                    pool = self.pool_for(window, ordinal)
                    limit = (len(self.quaternions) if pool == POOL_QUATERNIONS
                             else len(self.moves))
                    vals = self.quaternion_indices[tr.start:end]
                    if vals and max(vals) >= limit:
                        problems.append(
                            f"sequence {si} {window} slot {slot} track "
                            f"{ordinal} value {max(vals)} >= {pool} count "
                            f"{limit}")
        return problems


def clip_slot_table(a, name: str) -> list:
    """Human-readable ``clip -> slot -> track -> pool`` table (list of lines)."""
    seq = a.sequence_by_name(name)
    if seq is None:
        return [f"no clip named {name!r}"]
    lines = ["clip %r: rot=%d mov=%d tc=%d key=%s time=%s" % (
        seq.name.strip(), seq.rotation_count, seq.movement_count,
        seq.time_count, seq.key_window, seq.time_window)]
    for window in WINDOWS:
        tracks = a.window_tracks(seq, window)
        static = 0
        rows = []
        for slot, ordinal in enumerate(tracks):
            tr = a.tracks[ordinal]
            count = 1 if tr.is_single else seq.time_count
            pool = a.pool_for(window, ordinal)
            vals = a.quaternion_indices[tr.start:tr.start + max(count, 0)]
            if tr.is_single and vals and vals[0] == 0:
                static += 1
                continue
            rows.append("      %-8s slot %-3d track %-4d flags 0x%04X "
                        "%-11s keys %-3d qi[%d..%d) %s"
                        % (window, slot, ordinal, tr.flags, pool, count,
                           tr.start, tr.start + count,
                           vals[:6] if vals else []))
        lines.append("   %s: %d slots (%d static single-key, %d keyed)"
                     % (window, len(tracks), static, len(rows)))
        lines.extend(rows)
    return lines


def report_file(path: str, want_json: bool, with_tracks: bool,
                quaternions: int | None = None, with_tree: bool = False,
                with_sequences: bool = False, with_clip: str | None = None):
    a = AnimSet(path)
    rec = a.as_dict(with_tracks=with_tracks or quaternions is not None
                    or with_sequences)
    if not want_json:
        s = rec["summary"]
        print(f"== {os.path.basename(path)}  ({len(a.xmd.data)} bytes)")
        if not s["has_animation_set"]:
            print("   no COMPRESSEDANIMATIONSET2 chunk "
                  f"(chunk names: {', '.join(n.name for n in a.xmd.nodes)})")
            return rec
        print(f"   sequences={s['sequences']}  tracks={s['tracks']}  "
              f"keyframe_times={s['keyframe_times']}  "
              f"quaternions={s['quaternions']}  moves={s['moves']}  "
              f"track_indices={s['track_indices']}  "
              f"quaternion_indices={s['quaternion_indices']}  "
              f"duration={s['duration']:.3f}s")
        names = s["sequence_names"]
        if names:
            print(f"   sequence names: {', '.join(names[:12])}"
                  + (" ..." if len(names) > 12 else ""))
        if quaternions is not None:
            q = a.quaternion(quaternions)
            print("   q%-4d = (%.5f, %.5f, %.5f, %.5f)  norm=%.5f"
                  % ((quaternions,) + q + (sum(c * c for c in q) ** 0.5,)))
        if with_tracks:
            for i, t in enumerate(a.tracks[:64]):
                print("   track %-3d start=%-5d flags=0x%04X%s%s"
                      % (i, t.start, t.flags,
                         " movement" if t.has_movement else "",
                         " single" if t.is_single else ""))
            if len(a.tracks) > 64:
                print(f"   ... {len(a.tracks) - 64} more tracks")
        if with_tree:
            for line in a.xmd.tree_lines():
                print("   " + line)
        if with_sequences:
            print("   #  name                    key[lo:hi)     rot  mov  "
                  "time[lo:hi)")
            for i, s in enumerate(a.sequences):
                klo, khi = s.key_window
                tlo, thi = s.time_window
                print("   %-3d %-22s [%4d,%4d)  %4d %4d  [%4d,%4d)"
                      % (i, s.name, klo, khi, s.rotation_count,
                         s.movement_count, tlo, thi))
        if with_clip:
            for line in clip_slot_table(a, with_clip):
                print(line)
    if want_json and with_clip:
        seq = a.sequence_by_name(with_clip)
        rec["clip"] = a.clip_bindings(seq) if seq is not None else None
    return rec


def _walk(root: str):
    for dirpath, _dirs, files in os.walk(root):
        for fn in sorted(files):
            if fn.lower().endswith((".xsa", ".xag")):
                yield os.path.join(dirpath, fn)


def inventory(root: str) -> list[dict]:
    out = []
    for p in _walk(root):
        try:
            a = AnimSet(p)
            s = a.summary()
            s.pop("sequence_names", None)
            out.append({"path": p, **s})
        except (XmdError, XsaError) as e:
            out.append({"path": p, "error": str(e)})
    return out


def check_files(paths: list[str]) -> int:
    rc = 0
    for p in paths:
        try:
            a = AnimSet(p)
            if not a.has_anim:
                print(f"skip {os.path.basename(p)}: not a "
                      "COMPRESSEDANIMATIONSET2 file")
                continue
            problems = a.validate()
        except (XmdError, XsaError) as e:
            print(f"FAIL {os.path.basename(p)}: {e}")
            rc = 1
            continue
        if problems:
            rc = 1
            print(f"FAIL {os.path.basename(p)}: {'; '.join(problems)}")
        else:
            s = a.summary()
            print(f"ok   {os.path.basename(p)}: {s['sequences']} sequences, "
                  f"{s['tracks']} tracks, {s['quaternions']} quaternions, "
                  f"{s['moves']} moves")
    return rc


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("path", nargs="?", help="a .xsa animation set")
    ap.add_argument("--inventory", metavar="DIR",
                    help="walk DIR for *.xsa/*.xag and summarise all")
    ap.add_argument("--check", action="store_true",
                    help="validate the animation set structure (exit 1 on "
                         "problems); use with a file or --inventory DIR")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--tracks", action="store_true",
                    help="print the per-track (start, flags) table")
    ap.add_argument("--sequences", action="store_true",
                    help="print the per-sequence key/time window table")
    ap.add_argument("--clip", metavar="NAME",
                    help="print a clip's slot -> track -> pool binding table")
    ap.add_argument("--quaternions", type=int, metavar="N",
                    help="print pooled rotation N as a float quaternion")
    ap.add_argument("--tree", action="store_true",
                    help="print the container node tree")
    args = ap.parse_args(argv)

    if args.check:
        targets = []
        if args.inventory:
            targets = list(_walk(args.inventory))
        elif args.path:
            targets = [args.path]
        else:
            ap.error("--check needs a .xsa path or --inventory DIR")
        if not targets:
            ap.error("no .xsa/.xag files found")
        return check_files(targets)

    if args.inventory:
        recs = inventory(args.inventory)
        if args.json:
            print(json.dumps(recs, indent=2))
        else:
            for r in recs:
                if "error" in r:
                    print(f"  ERR {r['path']}: {r['error']}")
                elif not r["has_animation_set"]:
                    print(f"  -- {os.path.basename(r['path'])} "
                          "(not an animation set)")
                else:
                    print(f"{r['tracks']:>6} tracks  {r['sequences']:>4} seq  "
                          f"{r['quaternions']:>7} q  "
                          f"{os.path.basename(r['path'])}")
        return 0

    if not args.path:
        ap.error("provide a .xsa path or --inventory DIR")
    rec = report_file(args.path, args.json, args.tracks,
                      quaternions=args.quaternions, with_tree=args.tree,
                      with_sequences=args.sequences, with_clip=args.clip)
    if args.json:
        print(json.dumps(rec, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
