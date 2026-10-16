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


def _seq_record(name, name2="", a=1, b=0x40, f20=0, f22=0x44, f24=0x44,
                f25=3, f26=0, f28=0, f2a=0, f2c=1, version=0x103,
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
    return {
        "DIRECTORYHEADER": (b"", 0),
        "QUATERNIONS": (_quats_payload(quats), len(quats)),
        "MOVES": (_moves_payload(moves), len(moves)),
        "TRACKINDICES": (_u16_payload([0, 1, 2, 0]), 4),
        "QUATERNIONINDICES": (_u16_payload([0, 1, 2, 0, 1]), 5),
        "KEYFRAMETIMES": (_f32_payload([0.0, 0.5, 1.0]), 3),
        "TRACKS": (_tracks_payload(tracks), len(tracks)),
        "DATAKEYS": (b"", 0),
        "SEQUENCES": (_seq_record("cut01") + _seq_record("cut02   "), 2),
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


def main() -> int:
    tests = (test_anim_roundtrip, test_non_anim_container_is_ok,
             test_detects_quaternion_tiling, test_detects_non_unit_quaternion,
             test_detects_unknown_track_flags,
             test_detects_track_start_out_of_range, test_detects_missing_chunk,
             test_detects_sequence_tiling,
             test_rejects_untested_sequence_version)
    with tempfile.TemporaryDirectory() as d:
        for t in tests:
            t(d)
            print(f"ok   {t.__name__}")
    print("PASS xsa_anim structural tests")
    return 0


if __name__ == "__main__":
    sys.exit(main())
