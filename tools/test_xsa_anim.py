#!/usr/bin/env python3
"""Self-contained structural test for ``tools/xsa_anim.py``.

Builds synthetic ``MOS DATAFILE2.0`` containers whose ``COMPRESSEDANIMATIONSET2``
node matches the layout recovered from MXR.dll (see
``docs/adr/0422-xsa-compressed-animation-set.md``), then asserts the decoder and
its validator agree. No game install and no input synthesis.

Run headlessly:  python tools/test_xsa_anim.py
Exit code 0 = pass, 1 = fail.
"""
from __future__ import annotations

import os
import struct
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mos_xmd        # noqa: E402
import xsa_anim       # noqa: E402
import test_mos_xmd   # noqa: E402  (reuse the container builder)


def _string(s: str) -> bytes:
    b = s.encode("latin1")
    out = struct.pack("<I", len(b)) + b
    out += b"\x00" * ((-len(out)) % xsa_anim.SEQ_ALIGN)
    return out


def _seq_record(name, name2="", a=1, b=0x40, f20=0, f22=2, f24=2,
                f25=1, f26=0, f28=2, f2a=0, f2c=0, version=0x103,
                version2=0x100) -> bytes:
    out = struct.pack("<IHH", version, a, b)
    out += _string(name) + _string(name2)
    out += struct.pack("<H", version2)
    out += struct.pack("<HHBBHHHH", f20, f22, f24, f25, f26, f28, f2a, f2c)
    return bytes(out)


def _quats_payload(quats) -> bytes:
    return b"".join(struct.pack("<4h", *q) for q in quats)


def _moves_payload(moves) -> bytes:
    return b"".join(struct.pack("<3f", *m) for m in moves)


def _tracks_payload(tracks) -> bytes:
    return b"".join(struct.pack("<HH", *t) for t in tracks)


def _u16_payload(values) -> bytes:
    return b"".join(struct.pack("<H", v) for v in values)


def _f32_payload(values) -> bytes:
    return b"".join(struct.pack("<f", v) for v in values)


def _default_chunks():
    quats = [(0, 0, 0, 32767), (11585, 0, 0, 30652), (0, 16384, 0, 28378)]
    moves = [(0.0, 0.0, 0.0), (1.5, -2.0, 3.25)]
    tracks = [(0, 0x8000), (1, 0x0000), (2, 0x8001)]
    # Two sequences whose key windows tile TRACKINDICES [0,4) and whose time
    # windows tile KEYFRAMETIMES [0,3), exactly as every shipped set does:
    #   cut01 -> keys [0,3) = rot 2 + mov 1, times [0,2)
    #   cut02 -> keys [3,4) = rot 1 + mov 0, times [2,3)
    seqs = (_seq_record("cut01", f20=0, f22=2, f24=2, f25=1, f26=0, f28=2)
            + _seq_record("cut02   ", f20=3, f22=4, f24=1, f25=0, f26=2,
                          f28=1))
    return {
        "DIRECTORYHEADER": (b"", 0),
        "QUATERNIONS": (_quats_payload(quats), len(quats)),
        "MOVES": (_moves_payload(moves), len(moves)),
        "TRACKINDICES": (_u16_payload([0, 1, 2, 0]), 4),
        "QUATERNIONINDICES": (_u16_payload([0, 1, 2, 0, 1]), 5),
        "KEYFRAMETIMES": (_f32_payload([0.0, 0.5, 1.0]), 3),
        "TRACKS": (_tracks_payload(tracks), len(tracks)),
        "DATAKEYS": (b"", 0),
        "SEQUENCES": (seqs, 2),
    }


CHUNK_ORDER = ("DIRECTORYHEADER", "QUATERNIONS", "MOVES", "TRACKINDICES",
               "QUATERNIONINDICES", "KEYFRAMETIMES", "TRACKS", "DATAKEYS",
               "SEQUENCES")


def _anim_container(chunks=None, order=CHUNK_ORDER) -> bytes:
    """root -> COMPRESSEDANIMATIONSET2 -> (DIRECTORYHEADER, QUATERNIONS, ...).

    Nodes are laid out as contiguous 0x30-byte records at file offset
    ``stride * (i + 1)``; siblings are chained so the linked walk reaches every
    node (as in the shipped ``.xsa`` containers)."""
    chunks = chunks or _default_chunks()
    stride = mos_xmd.NODE_RECORD_LEN

    def at(i):
        return stride * (i + 1)

    nodes = []
    payloads = []
    for j, name in enumerate(order):
        payload, count = chunks[name]
        nxt = at(j + 2) if j + 1 < len(order) else 0
        nodes.append((name, nxt, 0, len(payload), count, 0))
        payloads.append(payload)
    # The animation-set node sits at 0x30 and owns the chunk chain as children.
    animset = (xsa_anim.ANIMSET, 0, at(1), 0, 0, 0)
    return test_mos_xmd.build_container([animset] + nodes, [b""] + payloads)


def _write_tmp(blob: bytes) -> str:
    fd, path = tempfile.mkstemp(suffix=".xsa")
    with os.fdopen(fd, "wb") as f:
        f.write(blob)
    return path


def _decode(blob: bytes):
    path = _write_tmp(blob)
    try:
        return xsa_anim.AnimSet(path)
    finally:
        os.remove(path)


def test_anim_roundtrip(tmp_dir):
    a = _decode(_anim_container())
    assert a.has_anim
    assert len(a.quaternions) == 3 and a.quaternions[0] == (0, 0, 0, 32767)
    q = a.quaternion(0)
    assert q == (0.0, 0.0, 0.0, 1.0), q
    assert len(a.moves) == 2 and a.moves[1] == (1.5, -2.0, 3.25), a.moves
    assert a.keyframe_times == [0.0, 0.5, 1.0], a.keyframe_times
    assert [(t.start, t.flags) for t in a.tracks] == \
        [(0, 0x8000), (1, 0), (2, 0x8001)]
    assert a.tracks[0].is_single and not a.tracks[0].has_movement
    assert a.tracks[2].has_movement and a.tracks[2].is_single
    assert a.sequence_names() == ["cut01", "cut02   "], a.sequence_names()
    assert a.sequences_consumed == len(_default_chunks()["SEQUENCES"][0])
    # Sequence-table windows (CXR_Anim_Sequence::Read, FUN_100774f0).
    assert [(s.key_window, s.time_window) for s in a.sequences] == \
        [((0, 3), (0, 2)), ((3, 4), (2, 3))], \
        [(s.key_window, s.time_window) for s in a.sequences]
    s0 = a.sequences[0]
    assert (s0.track_start, s0.track_mid, s0.rotation_count,
            s0.movement_count) == (0, 2, 2, 1)
    assert s0.track_mid == s0.track_start + s0.rotation_count
    problems = a.validate()
    assert problems == [], problems
    s = a.summary()
    assert (s["quaternions"], s["moves"], s["tracks"], s["sequences"]) == \
        (3, 2, 3, 2), s
    assert abs(s["duration"] - 1.0) < 1e-6, s


def test_non_anim_container_is_ok(tmp_dir):
    nodes = [("EXTENDEDMODEL", 0, 0, 0, 0, 0)]
    blob = test_mos_xmd.build_container(nodes, [b""])
    a = _decode(blob)
    assert not a.has_anim
    assert a.validate() == []
    assert a.summary()["has_animation_set"] is False


def test_detects_quaternion_tiling(tmp_dir):
    chunks = _default_chunks()
    payload, count = chunks["QUATERNIONS"]
    chunks["QUATERNIONS"] = (payload + b"\x00\x00", count)   # extra 2 bytes
    a = _decode(_anim_container(chunks))
    problems = a.validate()
    assert any("QUATERNIONS payload" in p for p in problems), problems


def test_detects_non_unit_quaternion(tmp_dir):
    chunks = _default_chunks()
    chunks["QUATERNIONS"] = (_quats_payload([(16384, 0, 0, 0)]), 1)
    a = _decode(_anim_container(chunks))
    problems = a.validate()
    assert any("not unit length" in p for p in problems), problems


def test_detects_unknown_track_flags(tmp_dir):
    chunks = _default_chunks()
    chunks["TRACKS"] = (_tracks_payload([(0, 0x0002)]), 1)
    a = _decode(_anim_container(chunks))
    problems = a.validate()
    assert any("unknown flags" in p for p in problems), problems


def test_detects_track_start_out_of_range(tmp_dir):
    chunks = _default_chunks()
    chunks["TRACKS"] = (_tracks_payload([(0, 0x8000), (9, 0x0000)]), 2)
    a = _decode(_anim_container(chunks))
    problems = a.validate()
    assert any("start 9 >=" in p for p in problems), problems


def test_detects_missing_chunk(tmp_dir):
    chunks = _default_chunks()
    order = tuple(n for n in CHUNK_ORDER if n != "TRACKS")
    a = _decode(_anim_container(chunks, order=order))
    problems = a.validate()
    assert any("missing TRACKS" in p for p in problems), problems


def test_detects_sequence_tiling(tmp_dir):
    chunks = _default_chunks()
    payload, count = chunks["SEQUENCES"]
    chunks["SEQUENCES"] = (payload + b"\xAA", count)   # stray byte
    a = _decode(_anim_container(chunks))
    problems = a.validate()
    assert any("SEQUENCES tiling" in p for p in problems), problems


def test_rejects_untested_sequence_version(tmp_dir):
    chunks = _default_chunks()
    chunks["SEQUENCES"] = (_seq_record("x", version=0x101), 1)
    try:
        _decode(_anim_container(chunks))
    except xsa_anim.XsaError as e:
        assert "0x101" in str(e), e
        return
    raise AssertionError("non-0x103 sequence version was accepted")


def test_detects_sequence_time_window_gap(tmp_dir):
    chunks = _default_chunks()
    chunks["SEQUENCES"] = (
        _seq_record("a", f20=0, f22=2, f24=2, f25=1, f26=0, f28=1)
        + _seq_record("b", f20=3, f22=4, f24=1, f25=0, f26=2, f28=1), 2)
    a = _decode(_anim_container(chunks))
    problems = a.validate()
    assert any("KEYFRAMETIMES tiling leaves" in p for p in problems), problems


def test_detects_sequence_key_window_gap(tmp_dir):
    chunks = _default_chunks()
    chunks["SEQUENCES"] = (
        _seq_record("a", f20=0, f22=1, f24=1, f25=1, f26=0, f28=2)
        + _seq_record("b", f20=3, f22=4, f24=1, f25=0, f26=2, f28=1), 2)
    a = _decode(_anim_container(chunks))
    problems = a.validate()
    assert any("TRACKINDICES tiling leaves" in p for p in problems), problems


def test_detects_sequence_key_window_overlap(tmp_dir):
    chunks = _default_chunks()
    chunks["SEQUENCES"] = (
        _seq_record("a", f20=0, f22=2, f24=2, f25=1, f26=0, f28=2)
        + _seq_record("b", f20=2, f22=3, f24=1, f25=0, f26=2, f28=1), 2)
    a = _decode(_anim_container(chunks))
    problems = a.validate()
    assert any("TRACKINDICES tiling covers" in p for p in problems), problems


def test_detects_sequence_track_mid_mismatch(tmp_dir):
    chunks = _default_chunks()
    chunks["SEQUENCES"] = (
        _seq_record("a", f20=0, f22=5, f24=2, f25=1, f26=0, f28=2)
        + _seq_record("b", f20=3, f22=4, f24=1, f25=0, f26=2, f28=1), 2)
    a = _decode(_anim_container(chunks))
    problems = a.validate()
    assert any("track_mid" in p for p in problems), problems


def test_detects_track_index_out_of_range(tmp_dir):
    chunks = _default_chunks()
    chunks["TRACKINDICES"] = (_u16_payload([0, 1, 9, 0]), 4)
    a = _decode(_anim_container(chunks))
    problems = a.validate()
    assert any("TRACKINDICES max 9 >= TRACKS count 3" in p
               for p in problems), problems


def test_slot_binding_rotation(tmp_dir):
    a = _decode(_anim_container())
    s0 = a.sequences[0]
    # cut01 rotation slot 1 -> track 1 (flags 0), 2 keys == time_count.
    b = a.slot_binding(s0, xsa_anim.ROTATION_WINDOW, 1)
    assert (b.track, b.flags, b.pool, b.count, b.start) == \
        (1, 0x0000, xsa_anim.POOL_QUATERNIONS, 2, 1), b.as_dict()
    assert b.pool_indices == [1, 2], b.pool_indices
    assert b.is_rotation and not b.single
    # slot 0 -> track 0 (single) -> exactly one key.
    b0 = a.slot_binding(s0, xsa_anim.ROTATION_WINDOW, 0)
    assert b0.single and b0.count == 1 and b0.pool_indices == [0]
    # out of range
    try:
        a.slot_binding(s0, xsa_anim.ROTATION_WINDOW, 2)
    except IndexError:
        pass
    else:
        raise AssertionError("slot 2 should be out of range")


def test_movement_pool_routes_on_flag(tmp_dir):
    """0x0001 on a movement-window track routes keys to QUATERNIONS."""
    a = _decode(_anim_container())
    s0 = a.sequences[0]
    # track 2 = (2, 0x8001): single *and* movement-flagged -> QUATERNIONS.
    b = a.slot_binding(s0, xsa_anim.MOVEMENT_WINDOW, 0)
    assert b.track == 2 and b.pool == xsa_anim.POOL_QUATERNIONS, b.as_dict()
    assert b.pool_indices == [2]
    # A movement track without the flag routes to MOVES.
    assert a.pool_for(xsa_anim.MOVEMENT_WINDOW, 0) == xsa_anim.POOL_MOVES
    assert a.pool_for(xsa_anim.ROTATION_WINDOW, 0) == xsa_anim.POOL_QUATERNIONS


def test_validate_movement_pool_flag(tmp_dir):
    # Value 2 fits QUATERNIONS (3) but not MOVES (1): ok because flag 0x0001.
    chunks = _default_chunks()
    chunks["MOVES"] = (_moves_payload([(0.0, 0.0, 0.0)]), 1)
    a = _decode(_anim_container(chunks))
    assert a.validate() == [], a.validate()
    # Without the flag the same value must fall inside MOVES -> a problem.
    chunks = _default_chunks()
    chunks["MOVES"] = (_moves_payload([(0.0, 0.0, 0.0)]), 1)
    chunks["TRACKS"] = (_tracks_payload([(0, 0x8000), (1, 0x0000),
                                         (2, 0x8000)]), 3)
    a = _decode(_anim_container(chunks))
    problems = a.validate()
    assert any(">= moves count 1" in p for p in problems), problems


def test_sample_rotation_interpolates(tmp_dir):
    a = _decode(_anim_container())
    s0 = a.sequences[0]
    q1 = a.quaternion(1)
    first = a.sample_rotation(s0, 1, 0.0)
    assert all(abs(x - y) < 1e-4 for x, y in zip(first, q1)), (first, q1)
    mid = a.sample_rotation(s0, 1, 0.25)             # halfway 0.0 -> 0.5
    assert abs(sum(c * c for c in mid) ** 0.5 - 1.0) < 1e-9
    q2 = a.quaternion(2)
    expect = tuple((x + y) / 2.0 for x, y in zip(q1, q2))
    n = sum(c * c for c in expect) ** 0.5
    expect = tuple(c / n for c in expect)
    assert all(abs(x - y) < 1e-6 for x, y in zip(mid, expect)), (mid, expect)
    # Single-key slot is constant.
    assert a.sample_rotation(s0, 0, 0.9) == a.quaternion(0)


def test_clip_slot_table(tmp_dir):
    a = _decode(_anim_container())
    lines = xsa_anim.clip_slot_table(a, "cut01")
    joined = "\n".join(lines)
    assert "clip 'cut01'" in joined, joined
    assert "rotation: 2 slots" in joined, joined
    assert "movement: 1 slots" in joined, joined
    assert "track 1" in joined and "track 2" in joined, joined
    assert xsa_anim.clip_slot_table(a, "nope") == ["no clip named 'nope'"]


def main() -> int:
    tests = (test_anim_roundtrip, test_non_anim_container_is_ok,
             test_detects_quaternion_tiling, test_detects_non_unit_quaternion,
             test_detects_unknown_track_flags,
             test_detects_track_start_out_of_range, test_detects_missing_chunk,
             test_detects_sequence_tiling,
             test_rejects_untested_sequence_version,
             test_detects_sequence_time_window_gap,
             test_detects_sequence_key_window_gap,
             test_detects_sequence_key_window_overlap,
             test_detects_sequence_track_mid_mismatch,
             test_detects_track_index_out_of_range,
             test_slot_binding_rotation,
             test_movement_pool_routes_on_flag,
             test_validate_movement_pool_flag,
             test_sample_rotation_interpolates,
             test_clip_slot_table)
    with tempfile.TemporaryDirectory() as d:
        for t in tests:
            t(d)
            print(f"ok   {t.__name__}")
    print("PASS xsa_anim structural tests")
    return 0


if __name__ == "__main__":
    sys.exit(main())
