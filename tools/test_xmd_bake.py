#!/usr/bin/env python3
"""Self-contained test for ``tools/xmd_bake.py``.

Builds a synthetic ``MOS DATAFILE2.0`` container (no game install, no input
synthesis), bakes it into the native C++ header format, parses that header back
and asserts the emitted geometry/skin provenance is faithful to the decoded
cluster. Also exercises the ``--verify`` stale-detection path.

Run headlessly:  python tools/test_xmd_bake.py
Exit code 0 = pass, 1 = fail.
"""
from __future__ import annotations

import os
import re
import sys
import tempfile

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)
import mos_xmd  # noqa: E402
import xmd_bake  # noqa: E402
import xmd_export  # noqa: E402
import test_mos_xmd as tm  # noqa: E402
import test_xmd_export as te  # noqa: E402


_VERTEX_RE = re.compile(
    r"\{\s*\{\s*([-\d.eE]+)f,\s*([-\d.eE]+)f,\s*([-\d.eE]+)f\s*\},\s*"
    r"\{\s*([-\d.eE]+)f,\s*([-\d.eE]+)f,\s*([-\d.eE]+)f\s*\},\s*"
    r"\{\s*([-\d.eE]+)f,\s*([-\d.eE]+)f\s*\}\s*\}")
_INDEX_BLOCK_RE = re.compile(r"Indices\[\] = \{(.*?)\};", re.S)


def _parse_vertices(header: str) -> list[tuple[float, ...]]:
    return [tuple(float(g) for g in m.groups())
            for m in _VERTEX_RE.finditer(header)]


def _parse_indices(header: str) -> list[list[int]]:
    return [[int(t) for t in re.findall(r"\d+", block.split("};")[0])]
            for block in _INDEX_BLOCK_RE.findall(header)]


def test_format_float_literals(tmp_dir):
    assert xmd_bake._cf(0.0) == "0.0f"
    assert xmd_bake._cf(-0.0) == "0.0f"
    assert xmd_bake._cf(1.0) == "1.0f"
    assert xmd_bake._cf(-3.0) == "-3.0f"
    assert xmd_bake._cf(0.5) == "0.5f"
    lit = xmd_bake._cf(1.5259022e-05)
    assert lit.endswith("f") and ("e" in lit or "." in lit), lit


def test_bake_roundtrip(tmp_dir):
    path = tm._write_tmp(te._single_cluster())
    try:
        x = mos_xmd.Xmd(path)
        mesh = xmd_export.decode_cluster(x, 0)
        rec = xmd_bake.bake_mesh(path, 0, "Synthetic", "synth.xmd")
        header = xmd_bake.render_header([rec])

        verts = _parse_vertices(header)
        idx = _parse_indices(header)
        assert len(verts) == mesh.vertex_count == 4, len(verts)
        assert idx[0] == list(mesh.triangles), (idx[0], mesh.triangles)

        for got, want in zip(verts, mesh.positions):
            for c in range(3):
                assert abs(got[c] - want[c]) < 1e-3, (got, want)

        assert "kMeshCount" in header and '"Synthetic"' in header
        assert "unsigned short kMesh0_Indices[]" in header
        assert rec["bones_used"] == [0, 1, 2, 3, 5], rec["bones_used"]
        assert rec["uvs"] == 4 and rec["influences"] == 5, rec
    finally:
        os.remove(path)


def test_bake_without_uv(tmp_dir):
    path = tm._write_tmp(te._single_cluster(with_uv=False))
    try:
        rec = xmd_bake.bake_mesh(path, 0, "NoUv", "synth.xmd")
        header = xmd_bake.render_header([rec])
        verts = _parse_vertices(header)
        assert rec["uvs"] == 0 and len(verts) == 4
        # Untextured clusters fall back to zero UVs but keep valid vertices.
        assert all(v[6] == 0.0 and v[7] == 0.0 for v in verts), verts
    finally:
        os.remove(path)


def test_manifest_totals(tmp_dir):
    path = tm._write_tmp(te._single_cluster())
    try:
        recs = [xmd_bake.bake_mesh(path, 0, "A", "synth.xmd"),
                xmd_bake.bake_mesh(path, 0, "B", "synth.xmd")]
        man = xmd_bake.render_manifest(recs, "synthetic")
        assert man["totals"] == {"meshes": 2, "vertices": 8, "triangles": 4}, \
            man["totals"]
        assert man["rotations_available"] is False
        assert man["meshes"][0]["source_sha256"]
        assert man["coordinate_system"].startswith("engine model space")
    finally:
        os.remove(path)


def test_verify_detects_stale(tmp_dir):
    path = tm._write_tmp(te._single_cluster())
    cpp = os.path.join(tmp_dir, "out.h")
    try:
        assert xmd_bake.main(["--model", path, "--cluster", "0",
                              "--name", "Synthetic", "--cpp", cpp]) == 0
        assert xmd_bake.main(["--model", path, "--cluster", "0",
                              "--name", "Synthetic", "--cpp", cpp,
                              "--verify"]) == 0
        with open(cpp, encoding="utf-8") as f:
            text = f.read()
        with open(cpp, "w", encoding="utf-8", newline="\n") as f:
            f.write(text.replace("kMeshCount", "kMeshCountMutated"))
        assert xmd_bake.main(["--model", path, "--cluster", "0",
                              "--name", "Synthetic", "--cpp", cpp,
                              "--verify"]) == 1
    finally:
        os.remove(path)


def test_main_requires_output(tmp_dir):
    path = tm._write_tmp(te._single_cluster())
    try:
        try:
            xmd_bake.main(["--model", path, "--cluster", "0"])
        except SystemExit as e:
            assert e.code == 2
        else:
            raise AssertionError("expected SystemExit when nothing to do")
    finally:
        os.remove(path)


def main() -> int:
    tests = (test_format_float_literals, test_bake_roundtrip,
             test_bake_without_uv, test_manifest_totals,
             test_verify_detects_stale, test_main_requires_output)
    with tempfile.TemporaryDirectory() as d:
        for t in tests:
            t(d)
            print(f"ok   {t.__name__}")
    print("PASS xmd_bake tests")
    return 0


if __name__ == "__main__":
    sys.exit(main())
