#!/usr/bin/env python3
"""Self-contained structural test for ``tools/mos_xmd.py``.

Builds synthetic ``MOS DATAFILE2.0`` containers in a temp directory (no game
install, no input synthesis) whose layout matches the loader recovered from
MCCDyn.dll (see ``docs/adr/0414-mos-datafile2-loader-container.md``), then
asserts the parser and its validator agree.

Run headlessly:  python tools/test_mos_xmd.py
Exit code 0 = pass, 1 = fail.
"""
from __future__ import annotations

import os
import struct
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mos_xmd  # noqa: E402  (path injected above)

MAGIC = b"MOS DATAFILE2.0\x00"


def _node(name: str, sibling: int, child: int,
          data_offset: int, data_size: int, count: int = 0,
          flags: int = 0, name_len: int = mos_xmd.NODE_NAME_LEN) -> bytes:
    nm = name.encode("latin1")[:name_len].ljust(name_len, b"\x00")
    return nm + struct.pack("<6I", sibling, child, data_offset,
                            data_size, count, flags)


def build_container(nodes, payloads):
    """nodes: list of (name, sibling, child, size, count, flags) at 0x30 stride.

    Returns the container bytes. ``payloads`` maps a node index -> bytes and is
    concatenated in node order starting at ``data_start``."""
    stride = mos_xmd.NODE_RECORD_LEN
    node_start = stride
    data_start = node_start + stride * len(nodes)
    off = data_start
    data_offsets = []
    for _name, _sib, _child, size, _cnt, _flg in nodes:
        data_offsets.append((off, size))
        off += size
    recs = bytearray()
    for (name, sib, child, size, cnt, flg), (doff, _s) in zip(nodes, data_offsets):
        recs += _node(name, sib, child, doff, size, cnt, flg)
    root = _node("MOS DATAFILE2.0", node_start, 0, data_start, 0)
    body = b"".join(payloads)
    blob = root + bytes(recs) + body
    assert len(root) == stride and len(blob) == off, (len(blob), off)
    return blob


def _write_tmp(blob: bytes) -> str:
    fd, path = tempfile.mkstemp(suffix=".xmd")
    with os.fdopen(fd, "wb") as f:
        f.write(blob)
    return path


def test_roundtrip(tmp_dir):
    # root -> A -> (B, C): A has child B, B has sibling C.
    stride = mos_xmd.NODE_RECORD_LEN
    nodes = [
        ("CONTAINER", 0, 0x30 + stride, 1, 7, 0),      # @0x30
        ("CHILDB", 0x30 + 2 * stride, 0, 4, 0, 0x201),  # @0x60
        ("CHILDC", 0, 0, 3, 0, 0),                      # @0x90
    ]
    blob = build_container(nodes, [b"\xAA", b"\xBB\xBB\xBB\xBB", b"\xCC\xCC\xCC"])
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        assert len(x.nodes) == 3, len(x.nodes)
        assert [n.name for n in x.nodes] == ["CONTAINER", "CHILDB", "CHILDC"]
        assert x.root.next_sibling == x.node_start == stride
        assert x.root.data_offset == x.data_start == 0x30 + 3 * stride
        a, b, c = x.nodes
        assert (a.first_child, a.data_size, a.count) == (b.offset, 1, 7)
        assert (b.next_sibling, b.flags) == (c.offset, 0x201)
        walked = [n.name for n in x.walk_linked()]
        assert walked[1:] == ["CONTAINER", "CHILDB", "CHILDC"], walked
        problems = x.validate()
        assert problems == [], problems
    finally:
        os.remove(path)


def test_detects_dangling_pointer():
    stride = mos_xmd.NODE_RECORD_LEN
    nodes = [
        ("SOLO", 0, 0, 2, 0, 0),
    ]
    # Corrupt SOLO's child pointer to a non-node offset.
    blob = bytearray(build_container(nodes, [b"\x01\x02"]))
    struct.pack_into("<I", blob, stride + mos_xmd.NODE_NAME_LEN + 4, 0xDEAD)
    path = _write_tmp(bytes(blob))
    try:
        x = mos_xmd.Xmd(path)
        problems = x.validate()
        assert any("not a node" in p for p in problems), problems
    finally:
        os.remove(path)


def test_rejects_bad_magic(tmp_dir):
    path = _write_tmp(b"NOT A MOS FILE" + b"\x00" * 64)
    try:
        try:
            mos_xmd.Xmd(path)
        except mos_xmd.XmdError:
            return
        raise AssertionError("bad magic was accepted")
    finally:
        os.remove(path)


def main() -> int:
    tests = (test_roundtrip, test_rejects_bad_magic)
    with tempfile.TemporaryDirectory() as d:
        for t in tests:
            t(d)
            print(f"ok   {t.__name__}")
        test_detects_dangling_pointer()
        print("ok   test_detects_dangling_pointer")
    print("PASS mos_xmd structural tests")
    return 0


if __name__ == "__main__":
    sys.exit(main())
