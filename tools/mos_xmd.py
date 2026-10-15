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
# Node names live in a 0x18-byte field (see module docstring).
NODE_NAME_LEN = 0x18
NODE_RECORD_LEN = 0x30
NAME_RE = re.compile(rb"^[A-Za-z][A-Za-z0-9_ ]{1,23}$")
# Chunks whose payload we know how to sanity-check (first u32 == element count).
COUNT_PREFIX_CHUNKS = {"TRIANGLES", "PRIMITIVES", "EDGES", "EDGETRIS", "TRIEDGES"}


class XmdError(Exception):
    pass


def _u32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


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
        return problems


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
        "root": x.root.as_dict(),
        "node_count": len(x.nodes),
        "chunks": [n.as_dict() for n in x.nodes],
        "geometry": x.geometry_summary(),
        "problems": x.validate(),
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
    rec = report_file(args.path, args.json, args.strings, args.tree)
    if args.json:
        print(json.dumps(rec, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
