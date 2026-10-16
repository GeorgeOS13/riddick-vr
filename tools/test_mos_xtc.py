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


def _mip_block(block_start, mips, compressed, codec, colorspace):
    """Build a packed-block mip chain: ``u32 mip_offset[n]`` + per-mip header
    (0x2C compressed / 0x1C raw) + pixel data.  ``mips`` is a list of
    ``(width, height, data_bytes)``."""
    n = len(mips)
    hdrlen = 0x2C if compressed else 0x1C
    table = bytearray(4 * n)
    body = bytearray()
    cur = block_start + 4 * n
    for i, (w, h, data) in enumerate(mips):
        struct.pack_into("<I", table, 4 * i, cur)
        if compressed:
            hdr = [0x300, colorspace, len(data) | 0x10, w, h, 0x800, 0,
                   codec, len(data), 0, 0x10]
        else:
            hdr = [0x300, 0, len(data), w, h, 0x800, 0]
        body += struct.pack("<%dI" % len(hdr), *hdr)
        body += data
        cur += hdrlen + len(data)
    return bytes(table) + bytes(body)


def build_xtc(images, overrides=None) -> bytes:
    """images: list of dicts with name/width/height/compressed/color/size.

    An image may instead carry ``mips`` (a list of ``(w, h, data)`` tuples)
    plus optional ``codec``/``colorspace``; a real packed mip block is then
    emitted and the image ``size`` is derived from it.

    ``overrides`` maps image index -> {desc index: value} applied after the
    canonical descriptor so negative tests can corrupt fields."""
    overrides = overrides or {}
    # absolute data offsets: blob starts right after the 0x30 root record.
    blob_off = 0x30
    for img in images:
        if "mips" in img:
            hdrlen = 0x2C if img.get("compressed", True) else 0x1C
            img["_size"] = 4 * len(img["mips"]) + \
                sum(hdrlen + len(m[2]) for m in img["mips"])
        else:
            img["_size"] = img["size"]
    offsets = []
    off = blob_off
    for img in images:
        offsets.append(off)
        off += img["_size"]
    blob_size = off - blob_off

    recs = []
    for i, img in enumerate(images):
        n = img["_size"]
        base = img.get("base_mip_bytes",
                       img["mips"][0][2].__len__() if "mips" in img else n)
        desc = [0, 0, 0x300, 0, 0, 0, 0, 0x800, 0, 0, 0x102, 0]
        desc[0] = img.get("desc0", 0)
        desc[1] = img.get("mip_levels",
                          int(math.log2(max(img["width"], img["height"]))) + 1)
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
    body = bytearray(blob_size)
    for i, img in enumerate(images):
        if "mips" in img:
            blk = _mip_block(offsets[i], img["mips"],
                             img.get("compressed", True), img.get("codec", 0),
                             img.get("colorspace", 0x5000))
            assert len(blk) == img["_size"], (len(blk), img["_size"])
            j = offsets[i] - blob_off
            body[j:j + len(blk)] = blk
    return root + bytes(body) + directory + textures + imagedir


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


def _dxt1(c0, c1, bits=0):
    return struct.pack("<HHI", c0, c1, bits)


def _dxt5(a0, a1, abits, c0, c1, cbits=0):
    return bytes((a0, a1)) + abits.to_bytes(6, "little") + \
        struct.pack("<HHI", c0, c1, cbits)


def _dxt3(alpha_nibbles, c0, c1, cbits=0):
    a = bytearray(8)
    for i, nib in enumerate(alpha_nibbles):
        a[i // 2] |= (nib & 0xF) << (4 * (i % 2))
    return bytes(a) + struct.pack("<HHI", c0, c1, cbits)


def test_mip_chain_and_bc1_decode(tmp_dir):
    red = _dxt1(0xF800, 0x001F)     # c0 > c1, index 0 -> c0 = red
    blue = _dxt1(0x001F, 0xF800)    # c0 < c1, index 0 -> c0 = blue
    imgs = [{"name": "T", "width": 4, "height": 4, "mip_levels": 3,
             "mips": [(4, 4, red), (2, 2, blue)], "codec": 0}]
    path = _write_tmp(build_xtc(imgs))
    try:
        with mos_xtc.Xtc(path) as x:
            img = x.image("T")
            mips = x.read_mips(img)
            assert [m.codec_name() for m in mips] == ["DXT1", "DXT1"]
            assert [(m.width, m.height, m.size) for m in mips] == \
                [(4, 4, 8), (2, 2, 8)]
            w, h, rgba = x.decode(img, 0)
            assert (w, h) == (4, 4)
            assert rgba == bytes((255, 0, 0, 255)) * 16, list(rgba[:8])
            w, h, rgba = x.decode(img, 1)
            assert rgba == bytes((0, 0, 255, 255)) * 4, list(rgba[:8])
            assert x.summary()["mip_chain_ok_images"] == 1
            assert x.summary()["mip_codec_histogram"] == {"DXT1": 2}
    finally:
        os.remove(path)


def test_mip_bc3_and_empty_level(tmp_dir):
    green = _dxt5(0xFF, 0x00, 0, 0x07E0, 0x0000)  # alpha 255, index 0 = green
    imgs = [{"name": "G", "width": 4, "height": 4, "mip_levels": 3,
             "mips": [(4, 4, green), (2, 2, b"")], "codec": 4}]
    path = _write_tmp(build_xtc(imgs))
    try:
        with mos_xtc.Xtc(path) as x:
            img = x.image("G")
            mips = x.read_mips(img)
            assert mips[0].codec_name() == "DXT5"
            assert mips[1].empty and mips[1].size == 0
            w, h, rgba = x.decode(img, 0)
            assert rgba == bytes((0, 255, 0, 255)) * 16, list(rgba[:8])
            w, h, rgba = x.decode(img, 1)
            assert (w, h) == (2, 2) and len(rgba) == 16
            assert x.summary()["mip_chain_ok_images"] == 1
    finally:
        os.remove(path)


def test_mip_bc2_decode(tmp_dir):
    # explicit 4-bit alpha (all 0x8 -> 0x88) + a green colour block
    green = _dxt3([8] * 16, 0x07E0, 0x0000)
    imgs = [{"name": "B2", "width": 4, "height": 4, "mip_levels": 3,
             "mips": [(4, 4, green)], "codec": 2}]
    path = _write_tmp(build_xtc(imgs))
    try:
        with mos_xtc.Xtc(path) as x:
            img = x.image("B2")
            assert x.read_mips(img)[0].codec_name() == "DXT3"
            w, h, rgba = x.decode(img, 0)
            assert rgba == bytes((0, 255, 0, 0x88)) * 16, list(rgba[:8])
    finally:
        os.remove(path)


def test_mip_raw_image(tmp_dir):
    data = bytes((1, 2, 3, 4))
    imgs = [{"name": "N", "width": 1, "height": 1, "compressed": False,
             "color": False, "mip_levels": 1, "base_mip_bytes": 4,
             "format_flags": 0x4300102, "mips": [(1, 1, data)]}]
    path = _write_tmp(build_xtc(imgs))
    try:
        with mos_xtc.Xtc(path) as x:
            img = x.image("N")
            mips = x.read_mips(img)
            assert mips[0].codec_name() == "raw32" and mips[0].size == 4
            w, h, rgba = x.decode(img, 0)
            assert (w, h) == (1, 1) and rgba == data
            assert x.validate() == [], x.validate()
    finally:
        os.remove(path)


def test_decode_writes_png(tmp_dir):
    red = _dxt1(0xF800, 0x001F)
    imgs = [{"name": "P", "width": 4, "height": 4, "mip_levels": 3,
             "mips": [(4, 4, red)], "codec": 0}]
    path = _write_tmp(build_xtc(imgs))
    out = os.path.join(tmp_dir, "p.png")
    try:
        with mos_xtc.Xtc(path) as x:
            w, h, rgba = x.decode(x.image("P"), 0)
            mos_xtc.write_png(out, w, h, rgba)
        blob = open(out, "rb").read()
        assert blob[:8] == b"\x89PNG\r\n\x1a\n"
        assert struct.unpack(">II", blob[16:24]) == (4, 4)
    finally:
        os.remove(path)


def test_mip_chain_break_is_reported(tmp_dir):
    red = _dxt1(0xF800, 0x001F)
    imgs = [{"name": "B", "width": 4, "height": 4, "mip_levels": 3,
             "mips": [(4, 4, red), (2, 2, red)], "codec": 0}]
    blob = bytearray(build_xtc(imgs))
    # point the second mip table entry at a bogus offset
    struct.pack_into("<I", blob, 0x30 + 4, 0x777)
    path = _write_tmp(bytes(blob))
    try:
        with mos_xtc.Xtc(path) as x:
            mips, reason = x.try_read_mips(x.image("B"))
            assert mips is None and reason, reason
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
        test_mip_chain_and_bc1_decode, test_mip_bc3_and_empty_level,
        test_mip_bc2_decode, test_mip_raw_image, test_decode_writes_png,
        test_mip_chain_break_is_reported,
    )
    with tempfile.TemporaryDirectory() as d:
        for t in tests:
            t(d)
            print(f"ok   {t.__name__}")
    print("PASS mos_xtc structural tests")
    return 0


if __name__ == "__main__":
    sys.exit(main())
