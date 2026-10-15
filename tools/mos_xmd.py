#!/usr/bin/env python3
"""Read-only parser for Starbreeze "MOS DATAFILE2.0" ``.xmd`` model containers.

The retail Riddick (Escape from Butcher Bay) character/weapon models are
``MOS DATAFILE2.0`` files. Format recovered by static inspection (2026-10):

  header  : 0x30 bytes
      +0x00  char[16] magic "MOS DATAFILE2.0\\0"
      +0x18  u32      node-table start offset (always 0x30)
      +0x20  u32      data-region start offset (== first chunk data offset)
  nodes   : contiguous 0x30-byte records from ``node_start`` to ``data_start``
      +0x00  char[16] chunk name (NUL padded)
      +0x10  u32[8]   fields; empirically:
                          [2] next sibling node offset   (0 = end)
                          [3] first child node offset    (0 = none)
                          [4] data offset (absolute file offset)
                          [5] data size in bytes
                          [6] element/frame count
                          [7] flags (e.g. TVERTEXFRAMES = 0x201)
  data    : chunk payloads, laid out contiguously starting at ``data_start``.

This parser is intentionally dependency-free and strictly read-only: it never
writes next to the game install. It backs the native first-person hand/arm
investigation (docs/adr/0413) and the REA track.

Usage:
    python tools/mos_xmd.py <file.xmd> [--json] [--strings] [--tree]
    python tools/mos_xmd.py --inventory <root-dir> [--json]
"""

from __future__ import annotations

import argparse
import json
import os
import re
import struct
import sys

MAGIC = b"MOS DATAFILE2.0\x00"
NAME_RE = re.compile(rb"^[A-Za-z][A-Za-z0-9_ ]{1,15}$")
# Chunks whose payload we know how to sanity-check (first u32 == element count).
COUNT_PREFIX_CHUNKS = {"TRIANGLES", "PRIMITIVES", "EDGES", "EDGETRIS", "TRIEDGES"}


class XmdError(Exception):
    pass


def _u32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


class Node:
    __slots__ = ("offset", "name", "next_sibling", "first_child",
                 "data_offset", "data_size", "count", "flags", "raw")

    def __init__(self, offset, name, fields):
        self.offset = offset
        self.name = name
        self.raw = fields
        self.next_sibling = fields[2]
        self.first_child = fields[3]
        self.data_offset = fields[4]
        self.data_size = fields[5]
        self.count = fields[6]
        self.flags = fields[7]

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

    def _parse_header(self):
        b = self.data
        if len(b) < 0x30 or b[:16] != MAGIC:
            raise XmdError(f"{self.path}: not a MOS DATAFILE2.0 file")
        self.node_start = _u32(b, 0x18)
        self.data_start = _u32(b, 0x20)
        if self.node_start < 0x30 or self.data_start < self.node_start:
            raise XmdError(f"{self.path}: implausible header offsets")

    def _parse_nodes(self):
        b = self.data
        self.nodes: list[Node] = []
        self.by_offset: dict[int, Node] = {}
        off = self.node_start
        while off + 0x30 <= self.data_start:
            name = b[off:off + 16].rstrip(b"\x00")
            if not NAME_RE.match(name):
                self.notes.append(
                    f"node table ended early at 0x{off:X} (name {name!r})")
                break
            fields = struct.unpack_from("<8I", b, off + 16)
            node = Node(off, name.decode("ascii"), fields)
            self.nodes.append(node)
            self.by_offset[off] = node
            off += 0x30

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
            "bone_deform_vertices": boned.count if boned else 0,
            "has_skinning": bool(boned and nodes),
            "vertex_frame_bytes": verts.data_size if verts else 0,
        }
        if tri and 0 < tri.data_offset < len(self.data) - 4:
            declared = _u32(self.data, tri.data_offset)
            summary["triangle_count"] = declared
            summary["triangle_count_matches_header"] = (declared == tri.count)
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
                with_tree: bool) -> dict:
    x = Xmd(path)
    rec = {
        "path": path,
        "size": len(x.data),
        "node_start": x.node_start,
        "data_start": x.data_start,
        "node_count": len(x.nodes),
        "chunks": [n.as_dict() for n in x.nodes],
        "geometry": x.geometry_summary(),
        "notes": x.notes,
    }
    if with_strings:
        rec["strings"] = x.text_strings()
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
                            "surfaces": g["surfaces"]})
            except XmdError as e:
                out.append({"path": p, "error": str(e)})
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("path", nargs="?", help="a .xmd file")
    ap.add_argument("--inventory", metavar="DIR",
                    help="walk DIR for *.xmd and summarise all")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--strings", action="store_true")
    ap.add_argument("--tree", action="store_true")
    args = ap.parse_args(argv)

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
    rec = report_file(args.path, args.json, args.strings, args.tree)
    if args.json:
        print(json.dumps(rec, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
