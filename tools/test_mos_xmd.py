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


def _vf204_frame(verts, normals, mn=(0.0, 0.0, 0.0), mx=(100.0, 200.0, 300.0),
                 lead=1) -> bytes:
    """Build a VERTEXFRAMES payload: one version-0x204 frame + leading count."""
    n = len(verts)
    out = bytearray()
    out += struct.pack("<I", lead)
    out += struct.pack("<I", 0x204)          # frame version
    out += struct.pack("<I", 0)              # header20
    out += struct.pack("<3f", *mn)           # bounds_min
    out += struct.pack("<3f", *mx)           # bounds_max
    out += struct.pack("<3f", 0.0, 0.0, 0.0)  # aux
    out += struct.pack("<I", 0)              # header30
    out += struct.pack("<I", n)              # vertex count
    for (x, y, z) in verts:
        out += struct.pack("<3H", x, y, z)
    for (x, y, z) in normals:
        out += struct.pack("<3b", x, y, z)
    assert len(out) == 56 + 9 * n, len(out)
    return bytes(out)


def _triangles(indices) -> bytes:
    out = bytearray(struct.pack("<I", len(indices) // 3))
    for i in indices:
        out += struct.pack("<H", i)
    return bytes(out)


def _tv200_frame(uvs, mn=(0.0, 0.0), mx=(1.0, 1.0)) -> bytes:
    """Build a TVERTEXFRAMES payload: ``u32 n; 4* f32; u16 uv[n][2]``."""
    n = len(uvs)
    out = bytearray(struct.pack("<I", n))
    out += struct.pack("<2f", *mn)
    out += struct.pack("<2f", *mx)
    for (u, v) in uvs:
        out += struct.pack("<2H", u, v)
    assert len(out) == 20 + 4 * n, len(out)
    return bytes(out)


def _uv_container(vf_payload, uv_payload, tri_payload, tri_count):
    stride = mos_xmd.NODE_RECORD_LEN
    # contiguous 0x30 records: VERTEXFRAMES @0x30, TVERTEXFRAMES @0x60, TRIANGLES @0x90
    nodes = [
        ("VERTEXFRAMES", stride * 2, 0, len(vf_payload), 1, 0),
        ("TVERTEXFRAMES", stride * 3, 0, len(uv_payload), 1, 0x201),
        ("TRIANGLES", 0, 0, len(tri_payload), tri_count, 0),
    ]
    return build_container(nodes, [vf_payload, uv_payload, tri_payload])


def _geometry_container(vf_payload, tri_payload, tri_count):
    stride = mos_xmd.NODE_RECORD_LEN
    node1 = stride * 2  # VERTEXFRAMES @0x60 when preceded by a 0x30 root
    nodes = [
        ("VERTEXFRAMES", node1, 0, len(vf_payload), 1, 0),
        ("TRIANGLES", 0, 0, len(tri_payload), tri_count, 0),
    ]
    return build_container(nodes, [vf_payload, tri_payload])


def test_geometry_roundtrip(tmp_dir):
    verts = [(0, 0, 0), (65535, 0, 0), (0, 65535, 0), (0, 0, 65535)]
    normals = [(127, 0, 0), (0, 127, 0), (0, 0, -127), (0, 0, 0)]
    blob = _geometry_container(_vf204_frame(verts, normals),
                               _triangles([0, 1, 2, 0, 2, 3]), 2)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        frames = x.vertex_frames()
        assert len(frames) == 1, frames
        f = frames[0]
        assert f.version == 0x204 and f.count == 4, (f.version, f.count)
        assert f.bounds_min == (0.0, 0.0, 0.0)
        assert f.bounds_max == (100.0, 200.0, 300.0)
        # q=65535 -> max, q=0 -> min
        assert f.positions[0] == (0.0, 0.0, 0.0), f.positions[0]
        assert f.positions[1] == (100.0, 0.0, 0.0), f.positions[1]
        assert f.positions[2] == (0.0, 200.0, 0.0), f.positions[2]
        assert abs(f.positions[3][2] - 300.0) < 1e-3, f.positions[3]
        # normals are re-normalized to unit length (or zeroed)
        assert abs(f.normals[0][0] - 1.0) < 1e-6, f.normals[0]
        assert f.normals[3] == (0.0, 0.0, 0.0), f.normals[3]
        cnt, idx = x.triangle_indices()
        assert cnt == 2 and max(idx) == 3, (cnt, idx)
        clusters = x.cluster_meshes()
        assert len(clusters) == 1 and clusters[0]["index_in_range"] is True
        assert x.validate() == [], x.validate()
        assert x.geometry_stats()["decoded_vertices"] == 4
        assert x.geometry_stats()["decoded_triangles"] == 2
    finally:
        os.remove(path)


def test_geometry_detects_index_out_of_range(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)]
    normals = [(0, 0, 0)] * 4
    # index 4 is one past the last vertex (count 4)
    blob = _geometry_container(_vf204_frame(verts, normals),
                               _triangles([0, 1, 4]), 1)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        problems = x.validate()
        assert any(">= decoded vertex count" in p for p in problems), problems
    finally:
        os.remove(path)


def test_geometry_detects_size_mismatch(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0), (0, 1, 0)]
    normals = [(0, 0, 0)] * 3
    # declare 2 triangles but only supply 1
    good = _triangles([0, 1, 2, 0, 1, 2])
    bad = bytearray(good)
    struct.pack_into("<I", bad, 0, 5)
    blob = _geometry_container(_vf204_frame(verts, normals), bytes(bad), 1)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        problems = x.validate()
        assert any("TRIANGLES size" in p for p in problems), problems
    finally:
        os.remove(path)


def test_uv_roundtrip(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)]
    normals = [(0, 0, 0)] * 4
    uvs = [(0, 0), (65535, 0), (0, 65535), (65535, 65535)]
    uv_payload = _tv200_frame(uvs, mn=(0.25, 0.5), mx=(0.75, 1.0))
    blob = _uv_container(_vf204_frame(verts, normals), uv_payload,
                         _triangles([0, 1, 2, 0, 2, 3]), 2)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        frames = x.tvertex_frames()
        assert len(frames) == 1, frames
        f = frames[0]
        assert f.version == 0x200 and f.count == 4, (f.version, f.count)
        assert f.bounds_min == (0.25, 0.5), f.bounds_min
        assert f.bounds_max == (0.75, 1.0), f.bounds_max
        # q=0 -> min, q=65535 -> max
        assert f.uvs[0] == (0.25, 0.5), f.uvs[0]
        assert abs(f.uvs[1][0] - 0.75) < 1e-6 and f.uvs[1][1] == 0.5, f.uvs[1]
        assert f.uvs[2][0] == 0.25 and abs(f.uvs[2][1] - 1.0) < 1e-6, f.uvs[2]
        assert x.validate() == [], x.validate()
        c = x.cluster_meshes()[0]
        assert c["uv_count"] == 4 and c["uv_matches"] is True, c
        stats = x.geometry_stats()
        assert stats["uv_clusters"] == 1 and stats["decoded_uvs"] == 4, stats
    finally:
        os.remove(path)


def test_uv_detects_count_mismatch(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)]
    normals = [(0, 0, 0)] * 4
    # only 3 UVs for 4 vertices
    uv_payload = _tv200_frame([(0, 0), (1, 0), (0, 1)])
    blob = _uv_container(_vf204_frame(verts, normals), uv_payload,
                         _triangles([0, 1, 2]), 1)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        problems = x.validate()
        assert any("UV count 3 != vertex count" in p for p in problems), problems
    finally:
        os.remove(path)


def test_uv_detects_bad_tiling(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0), (0, 1, 0)]
    normals = [(0, 0, 0)] * 3
    good = bytearray(_tv200_frame([(0, 0), (1, 0), (0, 1)]))
    # bump the declared count so size no longer tiles the chunk
    struct.pack_into("<I", good, 0, 5)
    blob = _uv_container(_vf204_frame(verts, normals), bytes(good),
                         _triangles([0, 1, 2]), 1)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        problems = x.validate()
        assert any("UV frame" in p for p in problems), problems
    finally:
        os.remove(path)


def test_uv_empty_chunk_is_ok(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0), (0, 1, 0)]
    normals = [(0, 0, 0)] * 3
    blob = _uv_container(_vf204_frame(verts, normals), b"",
                         _triangles([0, 1, 2]), 1)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        assert x.tvertex_frames() == []
        assert x.validate() == [], x.validate()
        c = x.cluster_meshes()[0]
        assert c["uv_count"] is None and c["uv_matches"] is None, c
    finally:
        os.remove(path)


def _bonedeform(vertices, influences, matrix_map_id=None, matrix_bones=()):
    """Build a BONEDEFORM payload: vertex records, influence pool, matrix map.

    ``vertices`` is a list of ``(flags, influence_count, influence_start)`` and
    ``influences`` a list of ``(bone, weight_q)`` (weight = q / 255)."""
    out = bytearray(struct.pack("<I", len(vertices)))
    for (flags, cnt, start) in vertices:
        out += struct.pack("<BBH", flags, cnt, start)
    out += struct.pack("<I", len(influences))
    for (bone, wq) in influences:
        out += struct.pack("<BB", bone, wq)
    if matrix_map_id is not None:
        out += struct.pack("<i", matrix_map_id)
        out += struct.pack("<I", len(matrix_bones))
        out += bytes(matrix_bones)
    return bytes(out)


def _skin_container(vf_payload, bd_payload, bones):
    """VERTEXFRAMES + BONEDEFORM (+ NODES carrying ``bones`` skeleton nodes)."""
    stride = mos_xmd.NODE_RECORD_LEN
    nodes_payload = b"\x00" * (34 * bones)
    nodes = [
        ("VERTEXFRAMES", stride * 2, 0, len(vf_payload), 1, 0),
        ("BONEDEFORM", stride * 3, 0, len(bd_payload), 0, 0),
        ("NODES", 0, 0, len(nodes_payload), bones, 0),
    ]
    return build_container(nodes, [vf_payload, bd_payload, nodes_payload])


def test_skin_roundtrip(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)]
    normals = [(0, 0, 0)] * 4
    vrecs = [(0, 2, 0), (0, 1, 2), (0, 3, 3), (0, 1, 6)]
    infl = [(0, 255), (1, 128), (2, 255), (0, 64), (3, 64), (5, 128),
            (7, 255)]
    bd = _bonedeform(vrecs, infl, matrix_map_id=-1, matrix_bones=())
    blob = _skin_container(_vf204_frame(verts, normals), bd, bones=8)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        sk = x.bone_deform()
        assert sk.vertex_count == 4 and sk.influence_count == 7, sk.as_dict()
        assert sk.vertices[0].influence_count == 2
        assert sk.influences[1] == (1, 128 / 255.0), sk.influences[1]
        assert sk.max_bone() == 7
        assert sk.max_influences() == 3
        assert sk.matrix_map_id == -1 and sk.matrix_bones == []
        assert sk.exact is True and sk.size == len(bd), (sk.size, len(bd))
        assert x.validate() == [], x.validate()
        c = x.cluster_meshes()[0]
        assert c["skin_matches"] is True and c["max_bone"] == 7, c
        st = x.geometry_stats()
        assert st["skinned_clusters"] == 1, st
        assert st["decoded_influences"] == 7, st
        assert st["max_influences_per_vertex"] == 3, st
    finally:
        os.remove(path)


def test_skin_matrix_map_and_influences_of(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0), (0, 1, 0)]
    normals = [(0, 0, 0)] * 3
    vrecs = [(0, 1, 0), (0, 2, 1), (0, 1, 3)]
    infl = [(4, 200), (6, 55), (9, 255), (2, 100)]
    bd = _bonedeform(vrecs, infl, matrix_map_id=3, matrix_bones=b"\x01\x02\x09")
    blob = _skin_container(_vf204_frame(verts, normals), bd, bones=16)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        sk = x.bone_deform()
        assert sk.matrix_map_id == 3, sk.matrix_map_id
        assert sk.matrix_bones == [1, 2, 9], sk.matrix_bones
        assert sk.influences_of(1) == [(6, 55 / 255.0), (9, 1.0)], \
            sk.influences_of(1)
        assert x.validate() == [], x.validate()
    finally:
        os.remove(path)


def test_skin_detects_prefix_sum_mismatch(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0)]
    normals = [(0, 0, 0)] * 2
    # second vertex must start at 1 but claims 0
    vrecs = [(0, 1, 0), (0, 1, 0)]
    infl = [(0, 255), (1, 255)]
    bd = _bonedeform(vrecs, infl, matrix_map_id=-1)
    blob = _skin_container(_vf204_frame(verts, normals), bd, bones=4)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        problems = x.validate()
        assert any("influence_start" in p for p in problems), problems
    finally:
        os.remove(path)


def test_skin_detects_vertex_count_mismatch(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0), (0, 1, 0)]
    normals = [(0, 0, 0)] * 3
    # only 2 vertex records for 3 mesh vertices
    vrecs = [(0, 1, 0), (0, 1, 1)]
    infl = [(0, 255), (1, 255)]
    bd = _bonedeform(vrecs, infl, matrix_map_id=-1)
    blob = _skin_container(_vf204_frame(verts, normals), bd, bones=4)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        problems = x.validate()
        assert any("skin vertex count 2 != vertex count" in p for p in problems), \
            problems
    finally:
        os.remove(path)


def test_skin_detects_bone_out_of_range(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0)]
    normals = [(0, 0, 0)] * 2
    vrecs = [(0, 1, 0), (0, 1, 1)]
    infl = [(0, 255), (5, 255)]     # bone 5 with only 4 skeleton nodes
    bd = _bonedeform(vrecs, infl, matrix_map_id=-1)
    blob = _skin_container(_vf204_frame(verts, normals), bd, bones=4)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        problems = x.validate()
        assert any("bone index 5 >= skeleton node count 4" in p
                   for p in problems), problems
    finally:
        os.remove(path)


def test_skin_bad_matrix_map_tiling(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0)]
    normals = [(0, 0, 0)] * 2
    vrecs = [(0, 1, 0), (0, 1, 1)]
    infl = [(0, 255), (1, 255)]
    good = bytearray(_bonedeform(vrecs, infl, matrix_map_id=0,
                                 matrix_bones=b"\x01\x02"))
    struct.pack_into("<I", good, len(good) - 6, 5)   # claim 5 bones, supply 2
    blob = _skin_container(_vf204_frame(verts, normals), bytes(good), bones=4)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        problems = x.validate()
        assert any("matrix-map size" in p for p in problems), problems
    finally:
        os.remove(path)


def test_skin_absent_is_ok(tmp_dir):
    verts = [(0, 0, 0), (1, 0, 0), (0, 1, 0)]
    normals = [(0, 0, 0)] * 3
    # geometry-only container: no BONEDEFORM node
    blob = _geometry_container(_vf204_frame(verts, normals),
                               _triangles([0, 1, 2]), 1)
    path = _write_tmp(blob)
    try:
        x = mos_xmd.Xmd(path)
        assert x.bone_deform() is None
        assert x.validate() == [], x.validate()
        c = x.cluster_meshes()[0]
        assert c["skin"] is None and c["skin_matches"] is None, c
        assert x.geometry_stats()["skinned_clusters"] == 0
    finally:
        os.remove(path)


def main() -> int:
    tests = (test_roundtrip, test_rejects_bad_magic,
             test_geometry_roundtrip, test_geometry_detects_index_out_of_range,
             test_geometry_detects_size_mismatch,
             test_uv_roundtrip, test_uv_detects_count_mismatch,
             test_uv_detects_bad_tiling, test_uv_empty_chunk_is_ok,
             test_skin_roundtrip, test_skin_matrix_map_and_influences_of,
             test_skin_detects_prefix_sum_mismatch,
             test_skin_detects_vertex_count_mismatch,
             test_skin_detects_bone_out_of_range,
             test_skin_bad_matrix_map_tiling, test_skin_absent_is_ok)
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
