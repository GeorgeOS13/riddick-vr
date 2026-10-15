#!/usr/bin/env python3
"""Read-only parser for Starbreeze "MOS DATAFILE2.0" ``.xmd`` model containers.

The retail Riddick (Escape from Butcher Bay) character/weapon models are
``MOS DATAFILE2.0`` files. Layout recovered two ways:

  * by static inspection of the on-disk bytes, and
  * by decompiling the shipped loader ``CDataFile::Open`` (MCCDyn.dll
    @0x10032FB0) and ``CDataFile::ReadTree_r`` (@0x10032840) with Ghidra
    (see ``docs/adr/0414-mos-datafile2-loader-container.md``).

The container *is* a node tree, and the file header is simply the **root
node record**:

  root node : 0x30-byte record at file offset 0
      +0x00  char[0x18] name = "MOS DATAFILE2.0\\0" (NUL padded to 24)
      +0x18  u32        next sibling node offset   (0 = none) == first node
      +0x1C  u32        first child node offset     (0 = none)
      +0x20  u32        data offset (absolute file offset) == data region start
      +0x24  u32        data size in bytes
      +0x28  u32        element/frame count
      +0x2C  u32        flags (e.g. TVERTEXFRAMES = 0x201)
  node      : same 0x30-byte record shape, linked by sibling (+0x18) and
              child (+0x1C) offsets into an in-file tree.
  data      : chunk payloads; for the shipped models they tile the region
              ``[data_start, EOF)`` exactly (no gaps, no overlaps).

Because the root name field holds the magic, the loader's 16-byte magic
compare at file 0x00 reads the root node's first 16 name bytes. ``node_start``
and ``data_start`` below are therefore just ``root.next_sibling`` and
``root.data_offset``; they are not separate header fields.

This parser is intentionally dependency-free and strictly read-only: it never
writes next to the game install. It backs the native first-person hand/arm
investigation (docs/adr/0413) and the REA track.

Usage:
    python tools/mos_xmd.py <file.xmd> [--json] [--strings] [--tree]
    python tools/mos_xmd.py <file.xmd> --mesh [--cluster N] [--json]
    python tools/mos_xmd.py --inventory <root-dir> [--json]
    python tools/mos_xmd.py <file.xmd> --check
    python tools/mos_xmd.py --inventory <root-dir> --check

Geometry (recovered from MXR.dll ``CTriangleMeshCore::Read`` +
``CVertexFrame::Read`` + ``CVertexFrame204::Read`` + the TVERTEXFRAMES reader
``FUN_100807d0``, see ``docs/adr/0415-xmd-vertex-frame-geometry.md`` and
``docs/adr/0416-xmd-uv-tvertexframes.md``):

  VERTEXFRAMES : ``u32 frame_count`` (only when the model version is > 0x100),
                 then ``frame_count`` frames. Each decoded frame (version 0x204,
                 used by every shipped model) is::

                     u32  version            = 0x204
                     u32  header20           (unknown; likely a scale/flag)
                     f32  bounds_min[3]      (quantization lower bound)
                     f32  bounds_max[3]      (quantization upper bound)
                     f32  aux[3]             (third header vector, unread by paths)
                     u32  header30           (unknown; likely a scale/flag)
                     u32  vertex_count  n
                     u16  positions[n][3]    (x,y,z in 0..65535)
                     s8   normals[n][3]      (components in -127..127)

                 positions are dequantized with
                 ``min + q * (max - min) / 65535`` and normals with
                 ``c / 127`` re-normalized (exactly as the loader does).

  TRIANGLES    : ``u32 triangle_count`` then ``triangle_count`` *u16* triplets.

  TVERTEXFRAMES: one quantized UV frame, recovered from MXR.dll
                 ``FUN_100807d0`` (the version-0x200 branch, which every
                 shipped asset selects)::

                     u32  uv_count  n
                     f32  min_u
                     f32  min_v
                     f32  max_u
                     f32  max_v
                     u16  uv[n][2]           (u,v in 0..65535)

                 ``u = min_u + q_u * (max_u - min_u) / 65535`` (and *v*
                 likewise) with the same ``1.5259022e-05`` reciprocal. Size is
                 exactly ``20 + 4n``. A cluster with **no** UVs stores an empty
                 (``data_size == 0``) ``TVERTEXFRAMES`` node.

  BONEDEFORM   : per-vertex skinning, recovered from MXR.dll
                 ``FUN_1007d260`` (the embedded form selected whenever the
                 model version is > 0x100). Layout::

                     u32  vertex_count    N
                     N * { u8 flags; u8 influence_count; u16 influence_start }
                     u32  influence_count M
                     M * { u8 bone; u8 weight_q }        (weight = q / 255)
                     u32  matrix_map_id   (s32; 0xFFFFFFFF = none)
                     u32  matrix_bone_count B
                     B * u8               (bone matrix-map palette indices)

                 ``influence_start`` is the running prefix sum of
                 ``influence_count`` (the reader repairs it in place), so a
                 vertex's influences are ``influences[start:start+count]``.
                 A cluster with **no** skinning omits the ``BONEDEFORM`` node.
                 The trailing matrix-map block is present for every shipped
                 asset (model version 0x302); it is validated by ``--check``.

  Clusters     : one ``VERTEXFRAMES``/``TVERTEXFRAMES``/``TRIANGLES`` triple per
                 submesh, in traversal order; every index must be < the paired
                 frame's vertex count and a non-empty UV frame's count must
                 equal its position frame's count (validated by ``--check``).

This parser is intentionally dependency-free and strictly read-only: it never
writes next to the game install. It backs the native first-person hand/arm
investigation (docs/adr/0413) and the REA track.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import struct
import sys

MAGIC = b"MOS DATAFILE2.0\x00"
# Node names live in a 0x18-byte field (see module docstring).
NODE_NAME_LEN = 0x18
NODE_RECORD_LEN = 0x30
NAME_RE = re.compile(rb"^[A-Za-z][A-Za-z0-9_ ]{1,23}$")
# Chunks whose payload we know how to sanity-check (first u32 == element count).
COUNT_PREFIX_CHUNKS = {"TRIANGLES", "PRIMITIVES", "EDGES", "EDGETRIS", "TRIEDGES"}

# VERTEXFRAMES payload versions (recovered from MXR.dll, see ADR-0415). Every
# shipped model uses 0x204 (quantized u16 positions + i8 normals); 0x202/0x203
# share its shape but store raw (unpacked) records, and are implemented from the
# same decompilation though no shipped asset exercises them.
VERTEX_FRAME_VERSIONS = (0x200, 0x201, 0x202, 0x203, 0x204)
# Quantized-position readers we can fully decode from the decompiled loader.
QUANTIZED_VF_VERSIONS = (0x202, 0x203, 0x204)
# 1/65535: u16 position component -> fraction of the bounding-box extent.
_Q16_SCALE = 1.0 / 65535.0
# 1/127: i8 normal component scale (re-normalized after decode).
_I8_NORMAL_SCALE = 1.0 / 127.0
# TVERTEXFRAMES payload (MXR.dll FUN_100807d0). Every shipped asset takes the
# version-0x200 branch: one quantized UV frame, size == 20 + 4n (see docstring).
TVERTEX_FRAME_VERSION = 0x200


class XmdError(Exception):
    pass


class GeometryError(XmdError):
    pass


class VertexFrame:
    """One decoded ``VERTEXFRAMES`` frame (a static pose / blend target)."""

    __slots__ = ("version", "count", "bounds_min", "bounds_max", "aux",
                 "header_extra", "positions", "normals", "size", "exact")

    def __init__(self, version, count, bounds_min, bounds_max, aux,
                 header_extra, positions, normals, size, exact):
        self.version = version
        self.count = count
        self.bounds_min = bounds_min
        self.bounds_max = bounds_max
        self.aux = aux
        self.header_extra = header_extra
        self.positions = positions
        self.normals = normals
        self.size = size
        self.exact = exact

    def as_dict(self, with_geometry=False):
        d = {
            "version": self.version,
            "vertex_count": self.count,
            "bounds_min": list(self.bounds_min),
            "bounds_max": list(self.bounds_max),
            "size": self.size,
            "exact": self.exact,
        }
        if with_geometry:
            d["positions"] = [list(p) for p in self.positions]
            d["normals"] = [list(n) for n in self.normals]
        return d


def decode_vertex_frame(b: bytes, off: int) -> tuple[VertexFrame, int]:
    """Decode one ``VERTEXFRAMES`` frame at ``off``.

    Returns ``(frame, size)`` where ``size`` is the frame's byte length. Raises
    ``GeometryError`` for unsupported versions (0x200/0x201 store unquantized
    float data and are not exercised by any shipped asset)."""
    ver = _u32(b, off)
    if ver not in QUANTIZED_VF_VERSIONS:
        raise GeometryError(
            f"VERTEXFRAMES version 0x{ver:04x} not decodable (only 0x202-0x204)")
    p = off + 4
    extra20 = _u32(b, p)
    p += 4
    mn = struct.unpack_from("<3f", b, p)
    p += 12
    mx = struct.unpack_from("<3f", b, p)
    p += 12
    extra = None
    if ver == 0x202:
        extra = _u32(b, p)
        p += 4
    aux = struct.unpack_from("<3f", b, p)
    p += 12
    extra30 = _u32(b, p)
    p += 4
    n = _u32(b, p)
    p += 4
    pos_end = p + n * 6
    nrm_end = pos_end + n * 3
    size = nrm_end - off
    if nrm_end > len(b):
        raise GeometryError(
            f"VERTEXFRAMES frame at 0x{off:X} runs past EOF "
            f"(needs {size} bytes)")
    sx = (mx[0] - mn[0]) * _Q16_SCALE
    sy = (mx[1] - mn[1]) * _Q16_SCALE
    sz = (mx[2] - mn[2]) * _Q16_SCALE
    positions = []
    for i in range(n):
        qx, qy, qz = struct.unpack_from("<3H", b, p + i * 6)
        positions.append((mn[0] + qx * sx, mn[1] + qy * sy, mn[2] + qz * sz))
    normals = []
    for i in range(n):
        qx, qy, qz = struct.unpack_from("<3b", b, pos_end + i * 3)
        vx = qx * _I8_NORMAL_SCALE
        vy = qy * _I8_NORMAL_SCALE
        vz = qz * _I8_NORMAL_SCALE
        ln = (vx * vx + vy * vy + vz * vz) ** 0.5
        normals.append((0.0, 0.0, 0.0) if ln == 0.0
                       else (vx / ln, vy / ln, vz / ln))
    frame = VertexFrame(ver, n, mn, mx, aux, (extra20, extra, extra30),
                        positions, normals, size, exact=True)
    return frame, size


def decode_triangles(b: bytes, off: int, size: int) -> tuple[int, tuple[int, ...]]:
    """Decode a ``TRIANGLES`` payload: ``u32 count`` + ``count`` *u16* triplets."""
    cnt = _u32(b, off)
    need = 4 + cnt * 3 * 2
    if need != size:
        raise GeometryError(
            f"TRIANGLES size {size} != 4 + {cnt}*6 = {need}")
    return cnt, struct.unpack_from("<%dH" % (cnt * 3), b, off + 4)


class TVertexFrame:
    """One decoded ``TVERTEXFRAMES`` frame (a quantized UV set)."""

    __slots__ = ("version", "count", "bounds_min", "bounds_max", "uvs",
                 "size", "exact")

    def __init__(self, version, count, bounds_min, bounds_max, uvs, size, exact):
        self.version = version
        self.count = count
        self.bounds_min = bounds_min          # (min_u, min_v)
        self.bounds_max = bounds_max          # (max_u, max_v)
        self.uvs = uvs                        # list of (u, v)
        self.size = size
        self.exact = exact

    def as_dict(self, with_geometry=False):
        d = {
            "version": self.version,
            "uv_count": self.count,
            "bounds_min": list(self.bounds_min),
            "bounds_max": list(self.bounds_max),
            "size": self.size,
            "exact": self.exact,
        }
        if with_geometry:
            d["uvs"] = [list(uv) for uv in self.uvs]
        return d


def decode_tvertex_frame(b: bytes, off: int) -> tuple[TVertexFrame, int]:
    """Decode one ``TVERTEXFRAMES`` payload at ``off``.

    Mirrors the version-0x200 branch of ``FUN_100807d0`` in MXR.dll (the branch
    every shipped asset selects). Returns ``(frame, size)`` where ``size`` is
    the payload length ``20 + 4 * uv_count``. Raises ``GeometryError`` if the
    payload runs past EOF."""
    if off + 4 > len(b):
        raise GeometryError(
            f"TVERTEXFRAMES frame at 0x{off:X} is truncated (no count word)")
    n = _u32(b, off)
    p = off + 4
    mn = struct.unpack_from("<2f", b, p)
    p += 8
    mx = struct.unpack_from("<2f", b, p)
    p += 8
    size = 20 + 4 * n
    if off + size > len(b):
        raise GeometryError(
            f"TVERTEXFRAMES frame at 0x{off:X} runs past EOF (needs {size} bytes)")
    su = (mx[0] - mn[0]) * _Q16_SCALE
    sv = (mx[1] - mn[1]) * _Q16_SCALE
    uvs = []
    for i in range(n):
        qu, qv = struct.unpack_from("<2H", b, p + i * 4)
        uvs.append((mn[0] + qu * su, mn[1] + qv * sv))
    frame = TVertexFrame(TVERTEX_FRAME_VERSION, n, mn, mx, uvs, size, exact=True)
    return frame, size


def _u32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


def _i32(b: bytes, off: int) -> int:
    return struct.unpack_from("<i", b, off)[0]


# BONEDEFORM (embedded skinning) constants, recovered from MXR.dll FUN_1007d260
# + FUN_1007c150 (version 0x200 branch). Weights are stored as u8 and scaled by
# 1/255; influence indices are u8 (so at most 256 bones).
BONE_DEFORM_VERSION = 0x200
_BONE_WEIGHT_SCALE = 1.0 / 255.0
# The trailing matrix-map id is a signed word; this is the "no matrix map" value.
BONE_MATRIX_MAP_NONE = -1


class BoneVertex:
    """One per-vertex ``BONEDEFORM`` record: ``{u8 flags; u8 n; u16 start}``."""

    __slots__ = ("flags", "influence_count", "influence_start")

    def __init__(self, flags, influence_count, influence_start):
        self.flags = flags
        self.influence_count = influence_count
        self.influence_start = influence_start

    def as_dict(self):
        return {
            "flags": self.flags,
            "influence_count": self.influence_count,
            "influence_start": self.influence_start,
        }


class BoneDeform:
    """Decoded ``BONEDEFORM`` payload: per-vertex bone influences + matrix map."""

    __slots__ = ("vertex_count", "vertices", "influence_count", "influences",
                 "matrix_map_id", "matrix_bones", "size", "exact")

    def __init__(self, vertex_count, vertices, influence_count, influences,
                 matrix_map_id, matrix_bones, size, exact):
        self.vertex_count = vertex_count
        self.vertices = vertices                # list of BoneVertex
        self.influence_count = influence_count
        self.influences = influences            # list of (bone, weight)
        self.matrix_map_id = matrix_map_id      # signed, or None when absent
        self.matrix_bones = matrix_bones        # list of u8 palette indices
        self.size = size
        self.exact = exact

    def influences_of(self, vertex_index: int):
        """Return ``[(bone, weight), ...]`` for one vertex (weights sum to ~1)."""
        v = self.vertices[vertex_index]
        s = v.influence_start
        return self.influences[s:s + v.influence_count]

    def max_bone(self) -> int:
        return max((bone for bone, _w in self.influences), default=-1)

    def max_influences(self) -> int:
        return max((v.influence_count for v in self.vertices), default=0)

    def as_dict(self, with_skin=False):
        d = {
            "vertex_count": self.vertex_count,
            "influence_count": self.influence_count,
            "max_bone": self.max_bone(),
            "max_influences": self.max_influences(),
            "matrix_map_id": self.matrix_map_id,
            "matrix_bone_count": len(self.matrix_bones),
            "size": self.size,
            "exact": self.exact,
        }
        if with_skin:
            d["vertices"] = [v.as_dict() for v in self.vertices]
            d["influences"] = [[bone, w] for bone, w in self.influences]
            d["matrix_bones"] = list(self.matrix_bones)
        return d


def decode_bonedeform(b: bytes, off: int,
                      size: int | None = None) -> tuple[BoneDeform, int]:
    """Decode a ``BONEDEFORM`` payload at ``off``.

    Mirrors ``FUN_1007d260`` in MXR.dll: a vertex-record array, a shared
    influence pool addressed by prefix-sum starts, and a trailing bone
    matrix-map block. When ``size`` is given, the payload must tile exactly
    (``exact=True``). Returns ``(BoneDeform, consumed_size)``. Raises
    ``GeometryError`` when a structural invariant is violated."""

    def need(p: int, n: int, what: str):
        if p + n > len(b):
            raise GeometryError(f"BONEDEFORM {what} at 0x{p:X} runs past EOF")
        if size is not None and p + n > off + size:
            raise GeometryError(f"BONEDEFORM {what} at 0x{p:X} runs past chunk")

    need(off, 4, "vertex count")
    n = _u32(b, off)
    p = off + 4
    need(p, n * 4, "vertex records")
    vertices = []
    for i in range(n):
        flags, cnt, start = struct.unpack_from("<BBH", b, p + i * 4)
        vertices.append(BoneVertex(flags, cnt, start))
    p += n * 4

    need(p, 4, "influence count")
    m = _u32(b, p)
    p += 4
    need(p, m * 2, "influences")
    influences = []
    for i in range(m):
        bone = b[p + i * 2]
        weight = b[p + i * 2 + 1] * _BONE_WEIGHT_SCALE
        influences.append((bone, weight))
    p += m * 2

    # The vertex records must address the influence pool as a running prefix
    # sum, and together cover it exactly (the loader relies on this).
    running = 0
    for i, v in enumerate(vertices):
        if v.influence_start != running:
            raise GeometryError(
                f"BONEDEFORM vertex {i} influence_start {v.influence_start} "
                f"!= running {running}")
        running += v.influence_count
    if running != m:
        raise GeometryError(
            f"BONEDEFORM influences covered {running} != declared {m}")

    matrix_map_id = None
    matrix_bones: list[int] = []
    if size is not None:
        remain = off + size - p
        if remain:
            need(p, 8, "matrix-map header")
            matrix_map_id = _i32(b, p)
            count = _u32(b, p + 4)
            if 8 + count != remain:
                raise GeometryError(
                    f"BONEDEFORM matrix-map size {remain} != 8 + {count}")
            matrix_bones = list(b[p + 8:p + 8 + count])
            p += 8 + count
    consumed = p - off
    exact = size is None or consumed == size
    return BoneDeform(n, vertices, m, influences, matrix_map_id,
                      matrix_bones, consumed, exact), consumed


class Node:
    __slots__ = ("offset", "name", "next_sibling", "first_child",
                 "data_offset", "data_size", "count", "flags", "raw")

    def __init__(self, offset, name):
        self.offset = offset
        self.name = name
        self.raw = None
        self.next_sibling = 0
        self.first_child = 0
        self.data_offset = 0
        self.data_size = 0
        self.count = 0
        self.flags = 0

    @classmethod
    def from_fields(cls, offset, name, fields):
        n = cls(offset, name)
        n.raw = fields
        n.next_sibling, n.first_child = fields[0], fields[1]
        n.data_offset, n.data_size = fields[2], fields[3]
        n.count, n.flags = fields[4], fields[5]
        return n

    def as_dict(self):
        return {
            "offset": self.offset,
            "name": self.name,
            "next_sibling": self.next_sibling,
            "first_child": self.first_child,
            "data_offset": self.data_offset,
            "data_size": self.data_size,
            "count": self.count,
            "flags": self.flags,
        }


class Xmd:
    def __init__(self, path: str):
        self.path = path
        with open(path, "rb") as f:
            self.data = f.read()
        self.notes: list[str] = []
        self._parse_header()
        self._parse_nodes()

    def _read_node(self, off: int) -> Node:
        b = self.data
        name = b[off:off + NODE_NAME_LEN].rstrip(b"\x00")
        fields = struct.unpack_from("<6I", b, off + NODE_NAME_LEN)
        return Node.from_fields(off, name.decode("latin1"), fields)

    def _parse_header(self):
        b = self.data
        if len(b) < NODE_RECORD_LEN or b[:16] != MAGIC:
            raise XmdError(f"{self.path}: not a MOS DATAFILE2.0 file")
        # The "header" is the root node record at file offset 0: its 0x18-byte
        # name field holds the magic. node_start/data_start are just the root's
        # sibling pointer and its payload offset (they are not extra fields).
        self.root = self._read_node(0)
        self.node_start = self.root.next_sibling
        self.data_start = self.root.data_offset
        if self.node_start < NODE_RECORD_LEN or self.data_start < self.node_start:
            raise XmdError(f"{self.path}: implausible root node offsets")

    def _parse_nodes(self):
        b = self.data
        self.nodes: list[Node] = []
        self.by_offset: dict[int, Node] = {}
        off = self.node_start
        while off + NODE_RECORD_LEN <= self.data_start:
            name = b[off:off + NODE_NAME_LEN].rstrip(b"\x00")
            if not NAME_RE.match(name):
                self.notes.append(
                    f"node table ended early at 0x{off:X} (name {name!r})")
                break
            node = self._read_node(off)
            self.nodes.append(node)
            self.by_offset[off] = node
            off += NODE_RECORD_LEN

    # -- reporting -------------------------------------------------------
    def chunk(self, name: str):
        for n in self.nodes:
            if n.name == name:
                return n
        return None

    def geometry_summary(self):
        tri = self.chunk("TRIANGLES")
        boned = self.chunk("BONEDEFORM")
        nodes = self.chunk("NODES")
        verts = self.chunk("VERTEXFRAMES")
        surf = self.chunk("SURFACES")
        summary = {
            "triangle_count": None,
            "triangle_count_matches_header": None,
            "surface_count": surf.count if surf else 0,
            "surfaces": self.surface_names() if surf else [],
            "skeleton_node_count": nodes.count if nodes else 0,
            "bone_deform_vertices": None,   # filled from decoded skin below
            "has_skinning": bool(boned and nodes),
            "vertex_frame_bytes": verts.data_size if verts else 0,
        }
        if tri and 0 < tri.data_offset < len(self.data) - 4:
            declared = _u32(self.data, tri.data_offset)
            summary["triangle_count"] = declared
            summary["triangle_count_matches_header"] = (declared == tri.count)
        dec = self.geometry_stats()
        summary["clusters"] = dec["cluster_count"]
        summary["decoded_vertices"] = dec["decoded_vertices"]
        summary["decoded_triangles"] = dec["decoded_triangles"]
        summary["uv_clusters"] = dec["uv_clusters"]
        summary["decoded_uvs"] = dec["decoded_uvs"]
        summary["skinned_clusters"] = dec["skinned_clusters"]
        summary["bone_deform_vertices"] = dec["skinned_vertices"]
        summary["decoded_influences"] = dec["decoded_influences"]
        summary["max_influences_per_vertex"] = dec["max_influences_per_vertex"]
        summary["bad_clusters"] = dec["bad_clusters"]
        return summary

    def surface_names(self) -> list[str]:
        """Extract the per-surface name strings (e.g. RIDDICK_ARMSCAR_D)."""
        surf = self.chunk("SURFACES")
        if not surf:
            return []
        start = surf.data_offset
        end = min(start + surf.data_size, len(self.data))
        return [s for s in _printable(self.data[start:end], 6)
                if re.fullmatch(r"[A-Za-z0-9_ .\-]+", s)]

    def text_strings(self) -> list[str]:
        """All printable runs >= 5 chars anywhere in the file (metadata)."""
        return _printable(self.data, 5)

    def tree_lines(self) -> list[str]:
        lines: list[str] = []
        seen: set[int] = set()

        def walk(off: int, depth: int):
            if off == 0 or off not in self.by_offset or off in seen:
                return
            seen.add(off)
            n = self.by_offset[off]
            lines.append("  " * depth + f"{n.name}  @0x{n.offset:X} "
                         f"off=0x{n.data_offset:X} size={n.data_size} "
                         f"count={n.count}")
            child = n.first_child
            while child and child in self.by_offset and child not in seen:
                walk(child, depth + 1)
                child = self.by_offset[child].next_sibling

        root = self.chunk("EXTENDEDMODEL")
        if root:
            walk(root.offset, 0)
        else:
            for n in self.nodes:
                walk(n.offset, 0)
        return lines

    def walk_linked(self) -> list[Node]:
        """Walk the node tree the way the engine loader does: from the root
        record, following sibling (+0x18) then descending into child (+0x1C).

        Unlike the contiguous scan this makes no assumption that records are
        packed; it is the authoritative traversal (see ``ReadTree_r``)."""
        order: list[Node] = []
        seen: set[int] = set()

        def visit(node: Node):
            # Mirrors CDataFile::ReadTree_r: process node, descend into the
            # child subtree, then continue along this node's sibling chain.
            while node is not None:
                if node.offset in seen:
                    return
                seen.add(node.offset)
                order.append(node)
                child = node.first_child
                if child and child in self.by_offset:
                    visit(self.by_offset[child])
                nxt = node.next_sibling
                node = self.by_offset.get(nxt) if nxt else None

        visit(self.root)
        return order

    def validate(self) -> list[str]:
        """Structural self-check. Returns a list of problems (empty = consistent).

        Confirms (a) root/header agreement, (b) every sibling/child pointer
        targets a parsed node, (c) the linked walk reaches every scanned node,
        and (d) node payload regions tile ``[data_start, EOF)``."""
        problems: list[str] = []
        if self.root.next_sibling != self.node_start:
            problems.append("root.next_sibling != node_start")
        if self.root.data_offset != self.data_start:
            problems.append("root.data_offset != data_start")

        valid = set(self.by_offset)
        for n in self.nodes:
            for label, ptr in (("sibling", n.next_sibling),
                               ("child", n.first_child)):
                if ptr and ptr not in valid:
                    problems.append(
                        f"{n.name}@0x{n.offset:X}: {label} 0x{ptr:X} not a node")

        reached = {n.offset for n in self.walk_linked()}
        missed = valid - reached
        if missed:
            problems.append(
                "linked walk missed %d node(s): %s"
                % (len(missed), ", ".join(f"0x{o:X}" for o in sorted(missed))))

        regions = sorted((n.data_offset, n.data_size)
                         for n in self.nodes if n.data_size > 0)
        prev = self.data_start
        for off, size in regions:
            if off < prev:
                problems.append(f"overlapping payload @0x{off:X}")
            elif off > prev:
                problems.append(f"gap in payloads 0x{prev:X}..0x{off:X}")
            prev = max(prev, off + size)
        if regions and prev != len(self.data):
            problems.append(
                f"payloads end at 0x{prev:X}, file is 0x{len(self.data):X}")

        # Geometry cross-check (GH #55): each decoded VERTEXFRAMES/
        # TVERTEXFRAMES/TRIANGLES/BONEDEFORM cluster must agree - triangle
        # indices inside the vertex range, UV/vertex count agreement, skinning
        # vertex count agreement, and every referenced bone inside the skeleton.
        nodes_chunk = self.chunk("NODES")
        bone_total = nodes_chunk.count if nodes_chunk else 0
        for c in self.cluster_meshes():
            if c.get("error_kind") == "corrupt":
                problems.append(f"cluster {c['index']}: {c['error']}")
            elif c["index_in_range"] is False:
                problems.append(
                    f"cluster {c['index']}: triangle index {c['max_index']} "
                    f">= decoded vertex count")
            elif c.get("uv_matches") is False:
                problems.append(
                    f"cluster {c['index']}: UV count {c['uv_count']} != vertex "
                    f"count")
            elif c.get("skin_matches") is False:
                problems.append(
                    f"cluster {c['index']}: skin vertex count "
                    f"{c['skin_vertices']} != vertex count")
            elif (c.get("max_bone") is not None and bone_total
                    and c["max_bone"] >= bone_total):
                problems.append(
                    f"cluster {c['index']}: bone index {c['max_bone']} "
                    f">= skeleton node count {bone_total}")
        return problems

    # -- geometry ---------------------------------------------------------
    def vertex_frames(self, node: Node | None = None) -> list[VertexFrame]:
        """Decode the frames of one ``VERTEXFRAMES`` chunk.

        Mirrors ``CTriangleMeshCore::Read`` + ``CVertexFrame::Read`` in MXR.dll:
        when the model version is > 0x100 a single ``u32`` frame-count word is
        read before the per-frame payloads, so a leading word that is not itself
        a known frame version is treated as that header."""
        if node is None:
            node = self.chunk("VERTEXFRAMES")
        if node is None:
            return []
        b = self.data
        off, end = node.data_offset, node.data_offset + node.data_size
        if _u32(b, off) not in VERTEX_FRAME_VERSIONS:
            off += 4  # per-model frame-count word (model version > 0x100)
        frames: list[VertexFrame] = []
        while off < end:
            frame, size = decode_vertex_frame(b, off)
            if size <= 0 or off + size > end:
                raise GeometryError(
                    f"{node.name}@0x{node.offset:X}: frame tiling overflow")
            frames.append(frame)
            off += size
        if off != end:
            raise GeometryError(
                f"{node.name}@0x{node.offset:X}: frames end at 0x{off:X}, "
                f"chunk ends at 0x{end:X}")
        return frames

    def triangle_indices(self, node: Node | None = None):
        if node is None:
            node = self.chunk("TRIANGLES")
        if node is None:
            return 0, ()
        return decode_triangles(self.data, node.data_offset, node.data_size)

    def tvertex_frames(self, node: Node | None = None) -> list[TVertexFrame]:
        """Decode the UV frame of one ``TVERTEXFRAMES`` chunk.

        Mirrors ``FUN_100807d0`` in MXR.dll. An empty chunk (``data_size == 0``)
        means the cluster carries no UVs and decodes to ``[]``."""
        if node is None:
            node = self.chunk("TVERTEXFRAMES")
        if node is None or node.data_size == 0:
            return []
        b = self.data
        off, end = node.data_offset, node.data_offset + node.data_size
        frames: list[TVertexFrame] = []
        while off < end:
            frame, size = decode_tvertex_frame(b, off)
            if size <= 0 or off + size > end:
                raise GeometryError(
                    f"{node.name}@0x{node.offset:X}: UV frame tiling overflow")
            frames.append(frame)
            off += size
        if off != end:
            raise GeometryError(
                f"{node.name}@0x{node.offset:X}: UV frames end at 0x{off:X}, "
                f"chunk ends at 0x{end:X}")
        return frames

    def bone_deform(self, node: Node | None = None) -> BoneDeform | None:
        """Decode a ``BONEDEFORM`` chunk (see :func:`decode_bonedeform`).

        Returns ``None`` when the model/cluster carries no ``BONEDEFORM`` node
        (an unskinned, rigidly-bound cluster)."""
        if node is None:
            node = self.chunk("BONEDEFORM")
        if node is None:
            return None
        return decode_bonedeform(self.data, node.data_offset,
                                 node.data_size)[0]

    def _skin_owner_map(self) -> dict[int, Node]:
        """Map each ``VERTEXFRAMES`` node offset to its sibling ``BONEDEFORM``.

        In ``CTriangleMeshCore::Read`` a cluster's chunks are read as siblings
        (``VERTEXFRAMES`` -> ``TVERTEXFRAMES`` -> ``BONEDEFORM`` -> ...), so the
        ``BONEDEFORM`` that follows a ``VERTEXFRAMES`` in traversal order belongs
        to it. Clusters without skinning simply have no ``BONEDEFORM``."""
        owner: dict[int, Node] = {}
        last_vf: Node | None = None
        for n in self.walk_linked():
            if n.name == "VERTEXFRAMES":
                last_vf = n
            elif n.name == "BONEDEFORM" and last_vf is not None:
                owner.setdefault(last_vf.offset, n)
        return owner

    def cluster_meshes(self) -> list[dict]:
        """Pair ``VERTEXFRAMES``/``TVERTEXFRAMES``/``TRIANGLES`` chunks in order.

        Each model stores one such triple per cluster (submesh). Decoding
        geometrically cross-checks them: every triangle index must fall inside
        the paired frame's vertex range, and a non-empty UV frame must have the
        same count as its position frame."""
        order = self.walk_linked()
        vf_nodes = [n for n in order if n.name == "VERTEXFRAMES"]
        tv_nodes = [n for n in order if n.name == "TVERTEXFRAMES"]
        tri_nodes = [n for n in order if n.name == "TRIANGLES"]
        skin_owner = self._skin_owner_map()
        clusters = []
        for i, vn in enumerate(vf_nodes):
            entry: dict = {
                "index": i,
                "vertex_chunk": vn.as_dict(),
                "frames": [],
                "uv_chunk": None,
                "uv_count": None,
                "uv_matches": None,
                "skin_chunk": None,
                "skin": None,
                "skin_vertices": None,
                "skin_matches": None,
                "max_bone": None,
                "triangle_chunk": None,
                "triangle_count": None,
                "max_index": None,
                "index_in_range": None,
                "error": None,
                "error_kind": None,
            }
            try:
                frames = self.vertex_frames(vn)
                entry["frames"] = [f.as_dict() for f in frames]
            except GeometryError as e:
                entry["error"] = str(e)
                entry["error_kind"] = ("unsupported" if "not decodable" in str(e)
                                       else "corrupt")
                frames = []
            if i < len(tv_nodes):
                tv = tv_nodes[i]
                entry["uv_chunk"] = tv.as_dict()
                try:
                    uv_frames = self.tvertex_frames(tv)
                    if uv_frames:
                        entry["uv_count"] = uv_frames[-1].count
                        if frames:
                            entry["uv_matches"] = (
                                uv_frames[-1].count == frames[-1].count)
                except GeometryError as e:
                    entry["error"] = (entry["error"] or "") + f"; {e}"
                    entry["error_kind"] = "corrupt"
            bdn = skin_owner.get(vn.offset)
            if bdn is not None:
                entry["skin_chunk"] = bdn.as_dict()
                try:
                    skin = self.bone_deform(bdn)
                    entry["skin"] = skin.as_dict()
                    entry["skin_vertices"] = skin.vertex_count
                    entry["max_bone"] = skin.max_bone()
                    if frames:
                        entry["skin_matches"] = (
                            skin.vertex_count == frames[-1].count)
                except GeometryError as e:
                    entry["error"] = (entry["error"] or "") + f"; {e}"
                    entry["error_kind"] = "corrupt"
            if i < len(tri_nodes):
                tn = tri_nodes[i]
                entry["triangle_chunk"] = tn.as_dict()
                try:
                    cnt, idx = self.triangle_indices(tn)
                    entry["triangle_count"] = cnt
                    if idx:
                        entry["max_index"] = max(idx)
                    if frames:
                        n = frames[-1].count
                        entry["index_in_range"] = (
                            entry["max_index"] is None or entry["max_index"] < n)
                except GeometryError as e:
                    entry["error"] = (entry["error"] or "") + f"; {e}"
                    entry["error_kind"] = "corrupt"
            clusters.append(entry)
        return clusters

    def geometry_stats(self) -> dict:
        """Aggregate decoded-geometry totals + a pass/fail cross-check."""
        clusters = self.cluster_meshes()
        total_verts = 0
        total_tris = 0
        total_uvs = 0
        total_infl = 0
        total_skin_verts = 0
        uv_clusters = 0
        skinned = 0
        max_infl = 0
        bad = 0
        for c in clusters:
            if c["frames"]:
                total_verts += c["frames"][-1]["vertex_count"]
            if c["triangle_count"] is not None:
                total_tris += c["triangle_count"]
            if c["uv_count"] is not None:
                total_uvs += c["uv_count"]
                uv_clusters += 1
            if c.get("skin") is not None:
                skinned += 1
                total_infl += c["skin"]["influence_count"]
                total_skin_verts += c["skin_vertices"]
                max_infl = max(max_infl, c["skin"]["max_influences"])
            if (c["error"] or c["index_in_range"] is False
                    or c.get("uv_matches") is False
                    or c.get("skin_matches") is False):
                bad += 1
        return {
            "cluster_count": len(clusters),
            "decoded_vertices": total_verts,
            "decoded_triangles": total_tris,
            "uv_clusters": uv_clusters,
            "decoded_uvs": total_uvs,
            "skinned_clusters": skinned,
            "skinned_vertices": total_skin_verts,
            "decoded_influences": total_infl,
            "max_influences_per_vertex": max_infl,
            "bad_clusters": bad,
        }


def _printable(b: bytes, minlen: int) -> list[str]:
    out, cur = [], bytearray()
    for c in b:
        if 32 <= c < 127:
            cur.append(c)
        else:
            if len(cur) >= minlen:
                out.append(cur.decode("latin1"))
            cur = bytearray()
    if len(cur) >= minlen:
        out.append(cur.decode("latin1"))
    return out


def report_file(path: str, want_json: bool, with_strings: bool,
                with_tree: bool, with_mesh: bool = False,
                cluster: int | None = None) -> dict:
    x = Xmd(path)
    rec = {
        "path": path,
        "size": len(x.data),
        "node_start": x.node_start,
        "data_start": x.data_start,
        "root": x.root.as_dict(),
        "node_count": len(x.nodes),
        "chunks": [n.as_dict() for n in x.nodes],
        "geometry": x.geometry_summary(),
        "problems": x.validate(),
        "notes": x.notes,
    }
    if with_strings:
        rec["strings"] = x.text_strings()
    if with_mesh or cluster is not None:
        rec["clusters"] = x.cluster_meshes()
    if cluster is not None:
        vf_nodes = [n for n in x.walk_linked() if n.name == "VERTEXFRAMES"]
        if not 0 <= cluster < len(vf_nodes):
            raise XmdError(f"cluster {cluster} out of range "
                           f"(0..{len(vf_nodes) - 1})")
        rec["cluster_detail"] = [
            f.as_dict(with_geometry=True) for f in x.vertex_frames(vf_nodes[cluster])]
        tv_nodes = [n for n in x.walk_linked() if n.name == "TVERTEXFRAMES"]
        if cluster < len(tv_nodes):
            rec["uv_detail"] = [
                f.as_dict(with_geometry=True)
                for f in x.tvertex_frames(tv_nodes[cluster])]
        owner = x._skin_owner_map()
        if vf_nodes[cluster].offset in owner:
            sk = x.bone_deform(owner[vf_nodes[cluster].offset])
            rec["skin_detail"] = [sk.as_dict(with_skin=True)]
    if not want_json:
        print(f"== {os.path.basename(path)}  ({len(x.data)} bytes, "
              f"{len(x.nodes)} nodes, data@0x{x.data_start:X})")
        g = rec["geometry"]
        print(f"   surfaces({g['surface_count']}): {g['surfaces']}")
        print(f"   triangles={g['triangle_count']} "
              f"(header-match={g['triangle_count_matches_header']})  "
              f"skeleton_nodes={g['skeleton_node_count']}  "
              f"skinned_verts={g['bone_deform_vertices']}  "
              f"skinning={g['has_skinning']}")
        print(f"   clusters={g['clusters']}  decoded_verts={g['decoded_vertices']}"
              f"  decoded_tris={g['decoded_triangles']}"
              f"  uv_clusters={g['uv_clusters']}  decoded_uvs={g['decoded_uvs']}")
        print(f"   skinned_clusters={g['skinned_clusters']}  "
              f"decoded_influences={g['decoded_influences']}  "
              f"max_influences_per_vertex={g['max_influences_per_vertex']}  "
              f"bad_clusters={g['bad_clusters']}")
        if with_mesh:
            for c in rec.get("clusters", []):
                fr = c["frames"][-1] if c["frames"] else {}
                print(f"   cluster {c['index']}: verts={fr.get('vertex_count')} "
                      f"ver={fr.get('version') and hex(fr['version'])} "
                      f"tris={c['triangle_count']} max_index={c['max_index']} "
                      f"in_range={c['index_in_range']} "
                      f"uvs={c['uv_count']} uv_match={c['uv_matches']} "
                      f"skin={c['skin'] and c['skin']['influence_count']} "
                      f"skin_match={c['skin_matches']} max_bone={c['max_bone']}"
                      + (f"  ERR {c['error']}" if c["error"] else ""))
        if cluster is not None:
            for d in rec["cluster_detail"]:
                print(f"   cluster {cluster} frame ver=0x{d['version']:x} "
                      f"verts={d['vertex_count']} bounds=({d['bounds_min']} .. "
                      f"{d['bounds_max']})")
                for p in d["positions"][:8]:
                    print("      pos %8.3f %8.3f %8.3f" % tuple(p))
            for u in rec.get("uv_detail", []):
                print(f"   cluster {cluster} uv frame ver=0x{u['version']:x} "
                      f"uvs={u['uv_count']} bounds=({u['bounds_min']} .. "
                      f"{u['bounds_max']})")
                for uv in u["uvs"][:8]:
                    print("      uv  %8.4f %8.4f" % tuple(uv))
            for sk in rec.get("skin_detail", []):
                print(f"   cluster {cluster} skin verts={sk['vertex_count']} "
                      f"influences={sk['influence_count']} "
                      f"max_bone={sk['max_bone']} "
                      f"matrix_map_id={sk['matrix_map_id']} "
                      f"matrix_bones={sk['matrix_bone_count']}")
                for i, v in enumerate(sk["vertices"][:4]):
                    inf = sk["influences"][v["influence_start"]:
                                           v["influence_start"]
                                           + v["influence_count"]]
                    print("      v%-4d bones=%s" % (
                        i, ", ".join("%d:%.3f" % (b, w) for b, w in inf)))
        if with_tree:
            for line in x.tree_lines():
                print("   " + line)
        if with_strings:
            print("   strings: " + ", ".join(rec["strings"][:40]))
    return rec


def inventory(root: str) -> list[dict]:
    out = []
    for dirpath, _dirs, files in os.walk(root):
        for fn in sorted(files):
            if not fn.lower().endswith(".xmd"):
                continue
            p = os.path.join(dirpath, fn)
            try:
                x = Xmd(p)
                g = x.geometry_summary()
                out.append({"path": p, "size": len(x.data),
                            "nodes": len(x.nodes),
                            "triangles": g["triangle_count"],
                            "skeleton_nodes": g["skeleton_node_count"],
                            "skinned_verts": g["bone_deform_vertices"],
                            "skinned_clusters": g["skinned_clusters"],
                            "surfaces": g["surfaces"]})
            except XmdError as e:
                out.append({"path": p, "error": str(e)})
    return out


def check_files(paths: list[str]) -> int:
    """Validate container structure for each path. Returns process exit code."""
    rc = 0
    for p in paths:
        try:
            x = Xmd(p)
            problems = x.validate()
        except XmdError as e:
            print(f"FAIL {os.path.basename(p)}: {e}")
            rc = 1
            continue
        if problems:
            rc = 1
            print(f"FAIL {os.path.basename(p)}: {'; '.join(problems)}")
        else:
            print(f"ok   {os.path.basename(p)}: {len(x.nodes)} nodes, "
                  f"data 0x{x.data_start:X}..0x{len(x.data):X}")
    return rc


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("path", nargs="?", help="a .xmd file")
    ap.add_argument("--inventory", metavar="DIR",
                    help="walk DIR for *.xmd and summarise all")
    ap.add_argument("--check", action="store_true",
                    help="validate container structure (exit 1 on problems); "
                         "use with a file or --inventory DIR")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--strings", action="store_true")
    ap.add_argument("--tree", action="store_true")
    ap.add_argument("--mesh", action="store_true",
                    help="decode VERTEXFRAMES/TRIANGLES cluster pairs and "
                         "print per-cluster geometry")
    ap.add_argument("--cluster", type=int, metavar="N",
                    help="print the decoded frames (with positions) of "
                         "VERTEXFRAMES cluster N")
    args = ap.parse_args(argv)

    if args.check:
        targets = []
        if args.inventory:
            for dirpath, _dirs, files in os.walk(args.inventory):
                targets += [os.path.join(dirpath, f) for f in sorted(files)
                            if f.lower().endswith(".xmd")]
        elif args.path:
            targets = [args.path]
        else:
            ap.error("--check needs a .xmd path or --inventory DIR")
        if not targets:
            ap.error("no .xmd files found")
        return check_files(targets)

    if args.inventory:
        recs = inventory(args.inventory)
        if args.json:
            print(json.dumps(recs, indent=2))
        else:
            for r in recs:
                if "error" in r:
                    print(f"  ERR {r['path']}: {r['error']}")
                else:
                    print(f"{r['triangles']!s:>7} tri  {r['skeleton_nodes']!s:>4} bones  "
                          f"{r['size']:>9} B  {os.path.basename(r['path'])}")
        return 0

    if not args.path:
        ap.error("provide a .xmd path or --inventory DIR")
    rec = report_file(args.path, args.json, args.strings, args.tree,
                      with_mesh=args.mesh, cluster=args.cluster)
    if args.json:
        print(json.dumps(rec, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
