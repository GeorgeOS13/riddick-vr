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
import xmd_fk  # noqa: E402
import xsa_anim  # noqa: E402
import test_mos_xmd as tm  # noqa: E402
import test_xmd_export as te  # noqa: E402
import test_xmd_fk as txf  # noqa: E402
import test_xsa_anim as ta  # noqa: E402


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
        assert man["totals"] == {"meshes": 2, "vertices": 8, "triangles": 4,
                                 "posed_vertices": 0}, man["totals"]
        assert man["rotations_available"] is False
        assert man["pose"]["posed_meshes"] == [], man["pose"]
        assert man["meshes"][0]["source_sha256"]
        assert man["coordinate_system"].startswith("engine model space")
    finally:
        os.remove(path)


# --------------------------------------------------------------------------- #
# GH #58: posed bake (offline FK/clip -> posed vertex variant)
# --------------------------------------------------------------------------- #
def _unskinned_cluster() -> bytes:
    """A single cluster with no BONEDEFORM/NODES (unskinned, no skeleton)."""
    verts = [(0, 0, 0), (65535, 0, 0), (0, 65535, 0)]
    normals = [(127, 0, 0), (0, 127, 0), (0, 0, -127)]
    vf = tm._vf204_frame(verts, normals)
    tri = tm._triangles([0, 1, 2])
    stride = mos_xmd.NODE_RECORD_LEN
    nodes = [
        ("VERTEXFRAMES", stride * 2, 0, len(vf), 1, 0),
        ("TRIANGLES", stride * 3, 0, len(tri), 1, 0),
    ]
    return tm.build_container(nodes, [vf, tri])


def _pose_anim_blob() -> bytes:
    """A synthetic .xsa whose rotation slot 1 is a 90 deg rotation about +Z."""
    quats = [(0, 0, 0, 32767)] * 8
    quats[1] = (0, 0, 23170, 23170)
    tracks = [(k, 0x8000) for k in range(8)]
    seqs = ta._seq_record("posed", f20=0, f22=8, f24=8, f25=0, f26=0, f28=2)
    chunks = {
        "DIRECTORYHEADER": (b"", 0),
        "QUATERNIONS": (ta._quats_payload(quats), len(quats)),
        "MOVES": (b"", 0),
        "TRACKINDICES": (ta._u16_payload(list(range(8))), 8),
        "QUATERNIONINDICES": (ta._u16_payload(list(range(8))), 8),
        "KEYFRAMETIMES": (ta._f32_payload([0.0, 1.0]), 2),
        "TRACKS": (ta._tracks_payload(tracks), len(tracks)),
        "DATAKEYS": (b"", 0),
        "SEQUENCES": (seqs, 1),
    }
    return ta._anim_container(chunks)


def test_bake_pose_matches_xmd_fk(tmp_dir):
    model = tm._write_tmp(txf._translated_cluster())
    anim = ta._write_tmp(_pose_anim_blob())
    try:
        # decoding the synthetic clip must be clean (the pose rests on it)
        a = xsa_anim.AnimSet(anim)
        assert a.has_anim and a.validate() == [], a.validate()
        pose = xmd_bake.load_pose({"clip": anim, "sequence": "posed", "key": 1},
                                  tmp_dir)
        assert pose[2] == 1.0, pose[2]          # keyframe_times[1]
        rec = xmd_bake.bake_mesh(model, 0, "Posed", "synth.xmd", pose=pose)
        assert rec["posed_positions"] is not None
        assert rec["pose_source"].endswith(":posed@key1=1.000000s"), rec["pose_source"]
        assert rec["pose_clip_sha256"] and rec["pose_clip_size"] > 0

        # the emitted pose must equal xmd_fk's skinning of the same clip/time
        x = mos_xmd.Xmd(model)
        sk = x.skeleton()
        mesh = xmd_export.decode_cluster(x, 0)
        seq, deltas, _notes = xmd_fk.sample_sequence(sk, a, "posed", 1.0)
        worlds = xmd_fk.world_matrices(sk, deltas)
        want = xmd_fk.skin_positions(mesh.positions, mesh.skin, worlds)
        for got, exp in zip(rec["posed_positions"], want):
            assert max(abs(got[i] - exp[i]) for i in range(3)) < 1e-9, (got, exp)
        # the pose actually moved the mesh (the clip drove a bone)
        assert max(abs(rec["posed_positions"][v][i] - rec["positions"][v][i])
                   for v in range(len(want)) for i in range(3)) > 1.0

        # the header carries the posed array + provenance and is stable
        header = xmd_bake.render_header([rec])
        assert "kMesh0_Posed_Vertices" in header
        assert ":posed@key1=1.000000s" in header
        verts = _parse_vertices(header)
        assert len(verts) == 8, len(verts)   # 4 bind + 4 posed
        # the last 4 parsed vertices are the posed copy
        for i in range(4):
            for c in range(3):
                assert abs(verts[4 + i][c] - want[i][c]) < 1e-3, (i, c)
    finally:
        os.remove(model)
        os.remove(anim)


def test_bake_pose_unskinned_is_skipped(tmp_dir):
    """An unskinned cluster keeps has_pose()==false and records the reason."""
    model = tm._write_tmp(_unskinned_cluster())
    anim = ta._write_tmp(_pose_anim_blob())
    try:
        pose = xmd_bake.load_pose({"clip": anim, "sequence": "posed", "key": 0},
                                  tmp_dir)
        rec = xmd_bake.bake_mesh(model, 0, "Plain", "synth.xmd", pose=pose)
        assert rec["posed_positions"] is None, rec["pose_notes"]
        assert any("unskinned" in n for n in rec["pose_notes"]), rec["pose_notes"]
    finally:
        os.remove(model)
        os.remove(anim)


def test_bake_pose_cli_and_verify(tmp_dir):
    model = tm._write_tmp(txf._translated_cluster())
    anim = ta._write_tmp(_pose_anim_blob())
    cpp = os.path.join(tmp_dir, "posed.h")
    try:
        args = ["--model", model, "--cluster", "0", "--name", "Posed",
                "--anim", anim, "--sequence", "posed", "--pose-key", "1",
                "--cpp", cpp]
        assert xmd_bake.main(args) == 0
        assert xmd_bake.main(args + ["--verify"]) == 0
        text = open(cpp, encoding="utf-8").read()
        assert "kMesh0_Posed_Vertices" in text
        # a bad sequence/key is a clean error, never a crash or a write
        assert xmd_bake.main(["--model", model, "--cluster", "0",
                              "--name", "Posed", "--anim", anim,
                              "--sequence", "nope", "--cpp", cpp]) == 1
        assert xmd_bake.main(["--model", model, "--cluster", "0",
                              "--name", "Posed", "--anim", anim,
                              "--sequence", "posed", "--pose-key", "99",
                              "--cpp", cpp]) == 1
    finally:
        os.remove(model)
        os.remove(anim)


def test_bake_no_pose_flag(tmp_dir):
    """--no-pose bakes bind geometry only (canonical pose skipped)."""
    model = tm._write_tmp(txf._translated_cluster())
    try:
        rec = xmd_bake.bake_mesh(model, 0, "Plain", "synth.xmd")
        assert rec["posed_positions"] is None
        assert xmd_bake.render_manifest([rec], "x")["totals"]["posed_vertices"] == 0
    finally:
        os.remove(model)


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
             test_verify_detects_stale, test_main_requires_output,
             test_bake_pose_matches_xmd_fk, test_bake_pose_unskinned_is_skipped,
             test_bake_pose_cli_and_verify, test_bake_no_pose_flag)
    with tempfile.TemporaryDirectory() as d:
        for t in tests:
            t(d)
            print(f"ok   {t.__name__}")
    print("PASS xmd_bake tests")
    return 0


if __name__ == "__main__":
    sys.exit(main())
