#!/usr/bin/env python3
"""Self-contained structural tests for ``tools/mos_xtc.py`` (GH #61).

Builds synthetic ``MOS DATAFILE2.0`` ``.xtc`` texture packs in a temp directory
(no game install, no input synthesis) matching the layout recovered from the
shipped ``Content/{GUI,Models,Levels}.xtc`` packs (see
``docs/adr/0431-xtc-texture-pack-container.md``), then asserts the reader and
its validator agree.

Run headlessly:  python tools/test_mos_xtc.py
Exit code 0 = pass, 1 = fail.
"""
from __future__ import annotations

import math
import os
import struct
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mos_xtc  # noqa: E402  (path injected above)

MAGIC = b"MOS DATAFILE2.0\x00"
STRIDE = mos_xtc.NODE_RECORD_LEN
NAME_LEN = mos_xtc.NODE_NAME_LEN
MARKER = mos_xtc.IMAGE_MARKER


def _node(name: str, sibling: int, child: int,
          data_offset: int, data_size: int, count: int = 0,
          flags: int = 0) -> bytes:
    nm = name.encode("latin1")[:NAME_LEN].ljust(NAME_LEN, b"\x00")
    return nm + struct.pack("<6I", sibling, child, data_offset,
                            data_size, count, flags)


def _record(name: str, desc) -> bytes:
    nm = name.encode("ascii")
    pad = b" " * ((-len(nm)) % 4)
    return MARKER + struct.pack("<I", len(nm)) + nm + pad + \
        struct.pack("<%dI" % len(desc), *desc)


def build_xtc(images, overrides=None) -> bytes:
    """images: list of dicts with name/width/height/compressed/color/size.

    ``overrides`` maps image index -> {desc index: value} applied after the
    canonical descriptor so negative tests can corrupt fields."""
    overrides = overrides or {}
    # absolute data offsets: blob starts right after the 0x30 root record.
    blob_off = 0x30
    offsets = []
    off = blob_off
    for img in images:
        offsets.append(off)
        off += img["size"]
    blob_size = off - blob_off

    recs = []
    for i, img in enumerate(images):
        n = img["size"]
        base = img.get("base_mip_bytes", n)
        desc = [0, 0, 0x300, 0, 0, 0, 0, 0x800, 0, 0, 0x102, 0]
        desc[0] = img.get("desc0", 0)
        desc[1] = int(math.log2(max(img["width"], img["height"]))) + 1
        desc[3] = 0x5000 if img.get("color", True) else 0
        desc[4] = base | (0x10 if img.get("compressed", True) else 0)
        desc[5] = img["width"]
        desc[6] = img["height"]
        desc[8] = img.get("desc8", 0)
        desc[9] = offsets[i]
        desc[10] = img.get("format_flags", 0x102)
        desc[11] = img.get("desc11", 0)
        for k, v in overrides.get(i, {}).items():
            desc[k] = v
        if "line" not in img:
            recs.append(_record(img["name"], desc))
        else:
            recs.append(img["line"])
    directory = b"".join(recs)
    dir_off = blob_off + blob_size
    node_start = dir_off + len(directory)

    textures = _node("TEXTURES", node_start + STRIDE, 0, blob_off, blob_size,
                     count=len(images))
    imagedir = _node("IMAGEDIRECTORY5", 0, 0, dir_off, len(directory),
                     count=len(images))
    root = _node("MOS DATAFILE2.0", node_start, 0, 0, 0)
    body = bytes(blob_size)
    return root + body + directory + textures + imagedir


def _write_tmp(blob: bytes) -> str:
    fd, path = tempfile.mkstemp(suffix=".xtc")
    with os.fdopen(fd, "wb") as f:
        f.write(blob)
    return path


def _imgs():
    return [
        {"name": "Riddick_ArmScar_D", "width": 512, "height": 1024,
         "size": 0x400},
        {"name": "riddick_armscar_N", "width": 512, "height": 1024,
         "size": 0x200, "compressed": False, "color": False,
         "format_flags": 0x4300102, "desc11": 0x8000000},
        {"name": "Riddick_Eye_D", "width": 128, "height": 128, "size": 0x100},
    ]


def test_roundtrip(tmp_dir):
    blob = build_xtc(_imgs())
    path = _write_tmp(blob)
    try:
        with mos_xtc.Xtc(path) as x:
            assert [i.name for i in x.images] == [
                "Riddick_ArmScar_D", "riddick_armscar_N", "Riddick_Eye_D"]
            assert x.node_start == len(blob) - 2 * STRIDE
            assert x.textures.data_offset == 0x30
            assert x.images[0].data_offset == 0x30
            assert x.images[0].width == 512 and x.images[0].height == 1024
            assert x.images[0].mip_levels == 11
            assert x.images[0].compressed and x.images[0].color
            assert x.images[0].base_mip_bytes == 0x400
            assert x.images[0].stored_size == 0x400
            assert x.images[1].data_offset == 0x30 + 0x400
            assert x.images[1].stored_size == 0x200
            assert not x.images[1].compressed and not x.images[1].color
            assert x.images[2].mip_levels == 8
            assert x.images[2].data_offset == 0x30 + 0x600
            assert x.images[2].stored_size == 0x100
            assert x.validate() == [], x.validate()
    finally:
        os.remove(path)


def test_find(tmp_dir):
    path = _write_tmp(build_xtc(_imgs()))
    try:
        with mos_xtc.Xtc(path) as x:
            assert x.image("Riddick_Eye_D") is not None
            assert x.image("nope") is None
            assert x.summary()["image_count"] == 3
            assert x.blob_region() == (0x30, 0x30 + 0x700)
    finally:
        os.remove(path)


def test_empty_directory(tmp_dir):
    path = _write_tmp(build_xtc([]))
    try:
        with mos_xtc.Xtc(path) as x:
            assert x.images == []
            assert x.validate() == [], x.validate()
    finally:
        os.remove(path)


def test_rejects_bad_magic(tmp_dir):
    path = _write_tmp(b"NOT A MOS FILE" + b"\x00" * 64)
    try:
        try:
            mos_xtc.Xtc(path)
            assert False, "expected XtcError"
        except mos_xtc.XtcError:
            pass
    finally:
        os.remove(path)


def test_rejects_unsupported_variant(tmp_dir):
    # A single incorrectly-named node: not the flat TEXTURES/IMAGEDIRECTORY5.
    root = _node("MOS DATAFILE2.0", 0x30, 0, 0, 0)
    node = _node("SOURCE", 0, 0, 0x30, 0)
    path = _write_tmp(root + bytes(0x10) + node)
    try:
        try:
            mos_xtc.Xtc(path)
            assert False, "expected XtcError"
        except mos_xtc.XtcError as e:
            assert "unsupported XTC variant" in str(e), e
    finally:
        os.remove(path)


def test_detects_node_gap(tmp_dir):
    blob = bytearray(build_xtc(_imgs()))
    # corrupt TEXTURES payload size (node table starts after the directory)
    dir_off = 0x30 + 0x700
    node_start = dir_off + (len(blob) - dir_off - 2 * STRIDE)
    t_off = node_start
    struct.pack_into("<I", blob, t_off + NAME_LEN + 0x0C, 0x777)
    path = _write_tmp(bytes(blob))
    try:
        with mos_xtc.Xtc(path) as x:
            problems = x.validate()
            assert any("payload" in p for p in problems), problems
    finally:
        os.remove(path)


def test_detects_non_power_of_two_dimensions(tmp_dir):
    path = _write_tmp(build_xtc(_imgs(), {2: {5: 100}}))
    try:
        with mos_xtc.Xtc(path) as x:
            assert any("non-power-of-two" in p for p in x.validate())
    finally:
        os.remove(path)


def test_detects_mip_count_mismatch(tmp_dir):
    path = _write_tmp(build_xtc(_imgs(), {0: {1: 3}}))
    try:
        with mos_xtc.Xtc(path) as x:
            assert any("mip count" in p for p in x.validate())
    finally:
        os.remove(path)


def test_detects_offset_outside_blob(tmp_dir):
    path = _write_tmp(build_xtc(_imgs(), {1: {9: 0x999999}}))
    try:
        with mos_xtc.Xtc(path) as x:
            assert any("outside TEXTURES blob" in p for p in x.validate())
    finally:
        os.remove(path)


def test_detects_non_monotonic_offsets(tmp_dir):
    # make image 2 point before image 1
    path = _write_tmp(build_xtc(_imgs()))
    try:
        with mos_xtc.Xtc(path) as x:
            x.images[2].desc[9] = x.images[1].data_offset
            assert any("not increasing" in p for p in x.validate())
    finally:
        os.remove(path)


def test_detects_bad_desc_constant(tmp_dir):
    path = _write_tmp(build_xtc(_imgs(), {0: {2: 0x123}}))
    try:
        with mos_xtc.Xtc(path) as x:
            assert any("desc[2]" in p for p in x.validate())
    finally:
        os.remove(path)


def test_detects_directory_not_tiling(tmp_dir):
    # Suppress the final record's trailing bytes by shrinking declared size.
    path = _write_tmp(build_xtc(_imgs()))
    try:
        with mos_xtc.Xtc(path) as x:
            x.image_dir.data_size -= 4  # claimed directory is short
            x.images.pop()
            assert any("directory is" in p for p in x.validate())
    finally:
        os.remove(path)


def main() -> int:
    tests = (
        test_roundtrip, test_find, test_empty_directory,
        test_rejects_bad_magic, test_rejects_unsupported_variant,
        test_detects_node_gap, test_detects_non_power_of_two_dimensions,
        test_detects_mip_count_mismatch, test_detects_offset_outside_blob,
        test_detects_non_monotonic_offsets, test_detects_bad_desc_constant,
        test_detects_directory_not_tiling,
    )
    with tempfile.TemporaryDirectory() as d:
        for t in tests:
            t(d)
            print(f"ok   {t.__name__}")
    print("PASS mos_xtc structural tests")
    return 0


if __name__ == "__main__":
    sys.exit(main())
