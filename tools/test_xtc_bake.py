#!/usr/bin/env python3
"""Self-contained tests for ``tools/xtc_bake.py`` (GH #61).

Builds a synthetic ``MOS DATAFILE2.0`` ``.xtc`` pack (no game install, no input
synthesis), bakes one DXT1 mip into the native C++ texture header, parses that
header back and asserts the emitted blocks / reference pixels / checksum are
faithful to the independent ``mos_xtc`` decoder. Also exercises ``--verify``
stale detection and the format/name error paths.

Run headlessly:  python tools/test_xtc_bake.py
Exit code 0 = pass, 1 = fail.
"""
from __future__ import annotations

import os
import re
import struct
import sys
import tempfile

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)
import mos_xtc  # noqa: E402
import xtc_bake  # noqa: E402
import test_mos_xtc as tm  # noqa: E402

_BLOCK_RE = re.compile(r"kTexture\d+_Dxt1\[\] = \{(.*?)\};", re.S)


def _dxt1(c0, c1, bits=0):
    return struct.pack("<HHI", c0, c1, bits)


def _red_block():
    return _dxt1(0xF800, 0x001F)  # c0 > c1, index 0 -> red


def _write_pack(root: str, blob: bytes) -> str:
    """Write ``blob`` as ``<root>/Content/Models.xtc`` and return the path."""
    d = os.path.join(root, "Content")
    os.makedirs(d, exist_ok=True)
    path = os.path.join(d, "Models.xtc")
    with open(path, "wb") as f:
        f.write(blob)
    return path


def _parse_blocks(header: str) -> bytes:
    m = _BLOCK_RE.search(header)
    assert m, "no Dxt1 block array in header"
    return bytes(int(t, 16) for t in re.findall(r"0x([0-9A-Fa-f]{2}),", m.group(1)))


def test_bake_roundtrip(tmp_dir):
    red = _red_block()
    blob = tm.build_xtc([{"name": "T", "width": 4, "height": 4, "mip_levels": 3,
                          "mips": [(4, 4, red)], "codec": 0}])
    path = _write_pack(tmp_dir, blob)

    with mos_xtc.Xtc(path) as x:
        w, h, rgba = x.decode(x.image("T"), 0)

    rec = xtc_bake.bake_texture(path, "Content/Models.xtc", "T", 0)
    header = xtc_bake.render_header([rec])

    assert _parse_blocks(header) == red, "blocks must be baked verbatim"
    assert rec["width"] == w == 4 and rec["height"] == h == 4
    assert rec["dxt1_bytes"] == 8
    assert rec["codec"] == "DXT1"
    assert rec["rgba_checksum"] == sum(rgba) & 0xFFFFFFFF
    for s in rec["samples"]:
        o = (s["y"] * 4 + s["x"]) * 4
        assert s["rgba"] == list(rgba[o:o + 4]), (s, rgba[o:o + 4])
    assert rec["samples"][0]["rgba"] == [255, 0, 0, 255]

    assert "native_hand_texture.h" in header
    assert '"T"' in header and "kTextureCount" in header
    assert "content/Models.xtc" not in header  # never lower-cased
    assert "Content/Models.xtc" in header


def test_manifest_metadata(tmp_dir):
    red = _red_block()
    blob = tm.build_xtc([{"name": "T", "width": 4, "height": 4, "mip_levels": 3,
                          "mips": [(4, 4, red)], "codec": 0}])
    path = _write_pack(tmp_dir, blob)
    rec = xtc_bake.bake_texture(path, "Content/Models.xtc", "T", 0)
    man = xtc_bake.render_manifest([rec], "synthetic")
    assert man["totals"] == {"textures": 1, "bytes": 8}, man["totals"]
    assert man["textures"][0]["dxt1_sha256"]
    assert man["textures"][0]["rgba_checksum"] == rec["rgba_checksum"]
    assert "DXT1" in man["codec"]


def test_cli_bake_and_verify(tmp_dir):
    red = _red_block()
    blob = tm.build_xtc([{"name": "T", "width": 4, "height": 4, "mip_levels": 3,
                          "mips": [(4, 4, red)], "codec": 0}])
    _write_pack(tmp_dir, blob)
    cpp = os.path.join(tmp_dir, "out.h")
    args = ["--game", tmp_dir, "--image", "T", "--mip", "0", "--cpp", cpp]
    assert xtc_bake.main(args) == 0
    assert xtc_bake.main(args + ["--verify"]) == 0
    with open(cpp, encoding="utf-8") as f:
        text = f.read()
    with open(cpp, "w", encoding="utf-8", newline="\n") as f:
        f.write(text.replace("kTextureCount", "kTextureCountMutated"))
    assert xtc_bake.main(args + ["--verify"]) == 1


def test_rejects_non_dxt1(tmp_dir):
    # A raw / uncompressed mip and a DXT5 mip are both out of scope.
    raw = tm.build_xtc([{"name": "N", "width": 4, "height": 4,
                         "compressed": False, "color": False, "mip_levels": 3,
                         "base_mip_bytes": 64,
                         "mips": [(4, 4, bytes(64))]}])
    path = _write_pack(tmp_dir, raw)
    try:
        xtc_bake.bake_texture(path, "Content/Models.xtc", "N", 0)
        assert False, "expected XtcError for a raw mip"
    except mos_xtc.XtcError as e:
        assert "raw" in str(e), e

    dxt5 = tm.build_xtc([{"name": "G", "width": 4, "height": 4, "mip_levels": 3,
                          "mips": [(4, 4, tm._dxt5(0xFF, 0x00, 0, 0x07E0, 0x0000))],
                          "codec": 4}])
    path = _write_pack(tmp_dir, dxt5)
    try:
        xtc_bake.bake_texture(path, "Content/Models.xtc", "G", 0)
        assert False, "expected XtcError for a DXT5 mip"
    except mos_xtc.XtcError as e:
        assert "DXT5" in str(e), e


def test_rejects_unknown_image_and_mip(tmp_dir):
    red = _red_block()
    blob = tm.build_xtc([{"name": "T", "width": 4, "height": 4, "mip_levels": 3,
                          "mips": [(4, 4, red)], "codec": 0}])
    path = _write_pack(tmp_dir, blob)
    try:
        xtc_bake.bake_texture(path, "Content/Models.xtc", "missing", 0)
        assert False, "expected XtcError for an unknown image"
    except mos_xtc.XtcError as e:
        assert "missing" in str(e), e
    try:
        xtc_bake.bake_texture(path, "Content/Models.xtc", "T", 9)
        assert False, "expected XtcError for an out-of-range mip"
    except mos_xtc.XtcError as e:
        assert "out of range" in str(e), e


def test_main_requires_output(tmp_dir):
    red = _red_block()
    blob = tm.build_xtc([{"name": "T", "width": 4, "height": 4, "mip_levels": 3,
                          "mips": [(4, 4, red)], "codec": 0}])
    _write_pack(tmp_dir, blob)
    try:
        xtc_bake.main(["--game", tmp_dir, "--image", "T", "--mip", "0"])
    except SystemExit as e:
        assert e.code == 2
    else:
        raise AssertionError("expected SystemExit when nothing to do")


def main() -> int:
    tests = (test_bake_roundtrip, test_manifest_metadata,
             test_cli_bake_and_verify, test_rejects_non_dxt1,
             test_rejects_unknown_image_and_mip, test_main_requires_output)
    with tempfile.TemporaryDirectory() as d:
        for t in tests:
            t(d)
            print(f"ok   {t.__name__}")
    print("PASS xtc_bake tests")
    return 0


if __name__ == "__main__":
    sys.exit(main())
