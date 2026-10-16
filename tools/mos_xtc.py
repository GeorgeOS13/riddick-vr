#!/usr/bin/env python3
"""Read-only ``MOS DATAFILE2.0`` ``.xtc`` texture-pack reader (GH #61).

The shipped Riddick textures live inside ``Content/{GUI,Models,Levels}.xtc``
(and ``ExtraContent/Offline01/Textures/Offline01.xtc``).  A ``.xtc`` is the
same ``CDataFile`` tree the ``.xmd`` models use (hence the identical
``"MOS DATAFILE2.0\\x00"`` magic), written by the engine's ``VirtualXTC``
texture container (``MSystem.dll``: ``CTextureContainer_VirtualXTC2`` /
``AddFromXTC`` / ``XTCCompile`` / ``".\\Raster\\MTextureContainerXTC2.cpp"``,
with diagnostics ``"Invalid XTC. (No IMAGELIST)"`` and
``"Invalid XTC. (IMAGELIST was not a directory.)"``).  That last string is the
give-away: the image directory is a chunk named ``IMAGEDIRECTORY5``.

Container layout (recovered from the three shipped packs, see ADR-0431)::

    root node  : 0x30-byte record at file offset 0
        +0x00  char[0x18] name = "MOS DATAFILE2.0\\0" (NUL padded)
        +0x18  u32 next_sibling = node_start (node table offset, near EOF)
        +0x1C  u32 child        = 0
        +0x20  u32 data_offset  = 0            (the root has no payload)
        +0x24  u32 data_size    = 0
        +0x28  u32 count        = 0
        +0x2C  u32 flags        = 0

    node       : the same 0x30-byte record shape, contiguous from
                 ``node_start`` (sibling +0x18, child +0x1C, payload offset
                 +0x20, payload size +0x24, count +0x28, flags +0x2C).  Every
                 shipped ``.xtc`` has exactly two, in this order:

        TEXTURES          payload = the packed texture-data blob, starting at
                          file offset 0x30 (right after the root record)
        IMAGEDIRECTORY5   payload = the image directory (the last region in
                          the file, immediately before the node table)

    The two node payloads tile ``[0x30, node_start)`` exactly.

Image-directory record (inside the ``IMAGEDIRECTORY5`` payload)::

    +0x00  u32 marker   = 0xFFFFFFFF
    +0x04  u32 zero     = 0
    +0x08  u32 name_len
    +0x0C  char name[name_len]           (no NUL terminator, ASCII)
    +...   pad with 0x20 (' ') to a 4-byte boundary
    +...   u32 desc[14+]                 (see below; always >= 48 bytes)

``desc`` is a flat little-endian ``u32`` array.  The leading fields are pinned
across all 4944 records in ``GUI/Models/Levels.xtc``::

    desc[0]  u32  0 (reserved / unknown)
    desc[1]  u32  mip level count == log2(max(w, h)) + 1     (verified 4944/4944)
    desc[2]  u32  0x300                                      (constant)
    desc[3]  u32  0x5000 (color) | 0x0 (linear / normal map)
    desc[4]  u32  base mip byte count | 0x10 when block-compressed
    desc[5]  u32  width  (power of two)
    desc[6]  u32  height (power of two)
    desc[7]  u32  unknown (0x800 typical, 0x20 for tiny textures)
    desc[8]  u32  0
    desc[9]  u32  absolute file offset of this image's packed block
                  (strictly increasing across records, inside TEXTURES)
    desc[10] u32  format/mip flags (0x102 color, 0x4300102 linear)
    desc[11] u32  0 (color) | 0x8000000 (linear) | 0x2000000 (small)

The record length is variable (58 bytes of descriptor for most records, up to
~300 for a handful that carry extra trailing data).  Records are framed by the
``FF FF FF FF 00 00 00 00`` marker, which we use to walk them; the walked
records tile the directory payload exactly.

The packed pixel data (the ``TEXTURES`` node payload):

Every image's packed block (at ``desc[9]``) is a **mip chain**::

    u32 mip_offset[mip_count]        absolute file offsets of each mip header
    per mip:  0x2C-byte header (compressed) | 0x1C-byte header (raw), data

``mip_count`` is read back from the table itself::

    mip_count = (mip_offset[0] - block_offset) / 4

(the table is exactly ``4 * mip_count`` bytes).  The per-mip header fields
(little-endian ``u32``)::

    compressed (0x2C header)             raw / linear (0x1C header)
      +0x00  0x300                         +0x00  0x300
      +0x04  colorspace (0x5000 or 0)      +0x04  colorspace (0)
      +0x08  mip bytes | 0x10              +0x08  mip bytes
      +0x0C  width                         +0x0C  width
      +0x10  height                        +0x10  height
      +0x14  0x800                         +0x14  0x800
      +0x18  0                             +0x18  0
      +0x1C  codec (0=DXT1, 2=DXT3,    (pixel data follows immediately)
                     4=DXT5)
      +0x20  mip bytes
      +0x24  0
      +0x28  0x10

The codec field (+0x1C) is **0 for DXT1 / BC1**, **2 for DXT3 / BC2** and
**4 for DXT5 / BC3**, and the stored mip ``bytes`` equals ``w*h/2`` (BC1) or
``w*h`` (BC2/BC3) exactly.  A mip with ``bytes == 0`` is an *empty* mip (a
header only, no pixel data).  Decoding the base mip as standard S3TC
reproduces the shipped textures (verified visually: BC1 on
``Riddick_ArmScar_D`` / ``Riddick_B03_D``, BC3 on ``Pa2_inmate_H04_D``, BC2 on
``SKYMOUNT_0``).  The engine reader this mirrors is
``CTextureContainer_VirtualXTC2::ReadTexture`` / ``GetTextureMipMap`` /
``CImage::ReadHeader`` in ``MSystem.dll`` (see ADR-0432).

Not claimed here (open questions, recorded in ADR-0431 / ADR-0432):
  * the exact pixel format of the raw (linear / normal-map) images -- they are
    32-bit (``bytes == w*h*4``) but the channel order is not verified;
  * the meaning of ``desc[0]``/``desc[7]``/``desc[10]`` bits;
  * why a handful of images ship a *reduced* or non-conforming mip chain
    (``mip_count`` < ``desc[1]``, e.g. ``SCam_D`` 9 of 10);
  * why the node ``count`` field (+0x28) is 2-14 higher than the number of
    framed records (e.g. Models.xtc says 914, frames 901); the framing tiles
    exactly, so the framed records are authoritative.

This tool is strictly read-only: it never writes to the game install.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import re
import struct
import sys

MAGIC = b"MOS DATAFILE2.0\x00"
NODE_NAME_LEN = 0x18
NODE_RECORD_LEN = 0x30
NAME_RE = re.compile(rb"^[A-Za-z][A-Za-z0-9_ ]{1,23}$")
# Directory record framing: marker then a zero u32.
IMAGE_MARKER = b"\xff\xff\xff\xff\x00\x00\x00\x00"
# Fixed descriptor prefix we decode (12 little-endian u32 = 48 bytes).  Every
# record has >= 58 descriptor bytes, so this is always in range.
DESC_U32 = 12
DESC_MIN_BYTES = 48
# Node names we expect in a texture pack.
TEXTURES = "TEXTURES"
IMAGEDIRECTORY = "IMAGEDIRECTORY5"
# Per-mip header length: compressed headers carry the S3TC codec field (+0x1C);
# raw/linear headers stop right after +0x18.
MIP_HEADER_COMPRESSED = 0x2C
MIP_HEADER_RAW = 0x1C
# S3TC codec codes (per-mip header +0x1C).
CODEC_DXT1 = 0
CODEC_DXT3 = 2
CODEC_DXT5 = 4
# Raw / linear 32-bit pixel blocks have no codec field.
CODEC_RAW = -1


class XtcError(Exception):
    pass


def _u32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


def _is_pow2(n: int) -> bool:
    return n > 0 and (n & (n - 1)) == 0


def _rgb565(color: int):
    return ((((color >> 11) & 0x1F) * 255) // 31,
            (((color >> 5) & 0x3F) * 255) // 63,
            ((color & 0x1F) * 255) // 31)


def _bc1_base(data: bytes, offset: int):
    """Return the 4-entry RGB palette and the 32-bit index word of one
    DXT1/BC1 block."""
    c0, c1, bits = struct.unpack_from("<HHI", data, offset)
    r0, g0, b0 = _rgb565(c0)
    r1, g1, b1 = _rgb565(c1)
    if c0 > c1:
        pal = [(r0, g0, b0), (r1, g1, b1),
               ((2 * r0 + r1) // 3, (2 * g0 + g1) // 3, (2 * b0 + b1) // 3),
               ((r0 + 2 * r1) // 3, (g0 + 2 * g1) // 3, (b0 + 2 * b1) // 3)]
    else:
        pal = [(r0, g0, b0), (r1, g1, b1),
               ((r0 + r1) // 2, (g0 + g1) // 2, (b0 + b1) // 2),
               (0, 0, 0)]
    return pal, bits


def bc1_decode(data: bytes, width: int, height: int) -> bytes:
    """Decode a DXT1 / BC1 surface to top-down RGBA8 bytes."""
    out = bytearray(width * height * 4)
    bw = max(1, (width + 3) // 4)
    bh = max(1, (height + 3) // 4)
    for by in range(bh):
        for bx in range(bw):
            o = (by * bw + bx) * 8
            if o + 8 > len(data):
                return bytes(out)
            pal, bits = _bc1_base(data, o)
            for py in range(4):
                for px in range(4):
                    x, y = bx * 4 + px, by * 4 + py
                    if x < width and y < height:
                        r, g, b = pal[(bits >> (2 * (4 * py + px))) & 3]
                        di = (y * width + x) * 4
                        out[di:di + 4] = bytes((r, g, b, 255))
    return bytes(out)


def bc2_decode(data: bytes, width: int, height: int) -> bytes:
    """Decode a DXT3 / BC2 surface to top-down RGBA8 bytes (explicit 4-bit
    alpha block first, colour block second)."""
    out = bytearray(width * height * 4)
    bw = max(1, (width + 3) // 4)
    bh = max(1, (height + 3) // 4)
    for by in range(bh):
        for bx in range(bw):
            o = (by * bw + bx) * 16
            if o + 16 > len(data):
                return bytes(out)
            pal, bits = _bc1_base(data, o + 8)
            for py in range(4):
                for px in range(4):
                    ci = (bits >> (2 * (4 * py + px))) & 3
                    nib = data[o + (4 * py + px) // 2]
                    ai = (nib >> (4 * (px % 2))) & 0xF
                    x, y = bx * 4 + px, by * 4 + py
                    if x < width and y < height:
                        r, g, b = pal[ci]
                        di = (y * width + x) * 4
                        out[di:di + 4] = bytes((r, g, b, ai * 17))
    return bytes(out)


def bc3_decode(data: bytes, width: int, height: int) -> bytes:
    """Decode a DXT5 / BC3 surface to top-down RGBA8 bytes (alpha block
    first, colour block second -- the shipped order)."""
    out = bytearray(width * height * 4)
    bw = max(1, (width + 3) // 4)
    bh = max(1, (height + 3) // 4)
    for by in range(bh):
        for bx in range(bw):
            o = (by * bw + bx) * 16
            if o + 16 > len(data):
                return bytes(out)
            a0, a1 = data[o], data[o + 1]
            abits = int.from_bytes(data[o + 2:o + 8], "little")
            apal = [a0, a1]
            if a0 > a1:
                for i in range(1, 7):
                    apal.append(((7 - i) * a0 + i * a1) // 7)
            else:
                for i in range(1, 5):
                    apal.append(((5 - i) * a0 + i * a1) // 5)
                apal += [0, 255]
            pal, bits = _bc1_base(data, o + 8)
            for py in range(4):
                for px in range(4):
                    ci = (bits >> (2 * (4 * py + px))) & 3
                    ai = (abits >> (3 * (4 * py + px))) & 7
                    x, y = bx * 4 + px, by * 4 + py
                    if x < width and y < height:
                        r, g, b = pal[ci]
                        di = (y * width + x) * 4
                        out[di:di + 4] = bytes((r, g, b, apal[ai]))
    return bytes(out)


def write_png(path: str, width: int, height: int, rgba: bytes) -> None:
    """Write 8-bit RGBA pixels to ``path`` as a PNG (stdlib only)."""
    import zlib

    def chunk(tag: bytes, body: bytes) -> bytes:
        return (struct.pack(">I", len(body)) + tag + body +
                struct.pack(">I", zlib.crc32(tag + body) & 0xFFFFFFFF))

    rows = bytearray()
    for y in range(height):
        rows.append(0)
        rows += rgba[y * width * 4:(y + 1) * width * 4]
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(rows), 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


class XtcNode:
    __slots__ = ("offset", "name", "next_sibling", "child", "data_offset",
                 "data_size", "count", "flags")

    def __init__(self, offset, name, fields):
        self.offset = offset
        self.name = name
        (self.next_sibling, self.child, self.data_offset, self.data_size,
         self.count, self.flags) = fields

    def as_dict(self):
        return {
            "offset": self.offset,
            "name": self.name,
            "next_sibling": self.next_sibling,
            "child": self.child,
            "data_offset": self.data_offset,
            "data_size": self.data_size,
            "count": self.count,
            "flags": self.flags,
        }


class XtcImage:
    """One framed record of the ``IMAGEDIRECTORY5`` payload."""

    __slots__ = ("name", "record_offset", "record_size", "desc_offset",
                 "desc", "stored_offset", "stored_size")

    def __init__(self, name, record_offset, record_size, desc_offset, desc):
        self.name = name
        self.record_offset = record_offset
        self.record_size = record_size
        self.desc_offset = desc_offset
        self.desc = desc
        self.stored_offset = 0
        self.stored_size = 0

    # -- decoded descriptor fields --------------------------------------
    @property
    def mip_levels(self) -> int:
        return self.desc[1]

    @property
    def color(self) -> bool:
        return self.desc[3] != 0

    @property
    def compressed(self) -> bool:
        return bool(self.desc[4] & 0x10)

    @property
    def base_mip_bytes(self) -> int:
        return self.desc[4] & ~0x10

    @property
    def width(self) -> int:
        return self.desc[5]

    @property
    def height(self) -> int:
        return self.desc[6]

    @property
    def data_offset(self) -> int:
        return self.desc[9]

    @property
    def format_flags(self) -> int:
        return self.desc[10]

    def as_dict(self):
        return {
            "name": self.name,
            "record_offset": self.record_offset,
            "record_size": self.record_size,
            "width": self.width,
            "height": self.height,
            "mip_levels": self.mip_levels,
            "color": self.color,
            "compressed": self.compressed,
            "base_mip_bytes": self.base_mip_bytes,
            "data_offset": self.data_offset,
            "stored_size": self.stored_size,
            "format_flags": self.format_flags,
            "desc": [hex(v) for v in self.desc],
        }


class XtcMip:
    """One mip level of an image's packed block (see the module docstring)."""

    __slots__ = ("level", "width", "height", "size", "codec", "compressed",
                 "data_offset", "header")

    def __init__(self, level, width, height, size, codec, compressed,
                 data_offset, header):
        self.level = level
        self.width = width
        self.height = height
        self.size = size
        self.codec = codec
        self.compressed = compressed
        self.data_offset = data_offset
        self.header = header

    @property
    def empty(self) -> bool:
        return self.size == 0

    def codec_name(self) -> str:
        if not self.compressed:
            return "raw32"
        return {CODEC_DXT1: "DXT1", CODEC_DXT3: "DXT3",
                CODEC_DXT5: "DXT5"}.get(self.codec, f"codec{self.codec}")

    def as_dict(self) -> dict:
        return {
            "level": self.level,
            "width": self.width,
            "height": self.height,
            "size": self.size,
            "compressed": self.compressed,
            "codec": self.codec,
            "codec_name": self.codec_name(),
            "empty": self.empty,
            "data_offset": self.data_offset,
            "header": [hex(v) for v in self.header],
        }


class Xtc:
    """Read-only parser for a shipped ``.xtc`` texture pack."""

    def __init__(self, path: str):
        self.path = path
        self.size = os.path.getsize(path)
        self.notes: list[str] = []
        self._fh = open(path, "rb")
        try:
            self._parse_header()
            self._parse_nodes()
            self._parse_directory()
        except Exception:
            self._fh.close()
            raise

    def close(self):
        self._fh.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    # -- container ------------------------------------------------------
    def _read_node(self, off: int) -> XtcNode:
        self._fh.seek(off)
        rec = self._fh.read(NODE_RECORD_LEN)
        if len(rec) < NODE_RECORD_LEN:
            raise XtcError(f"{self.path}: truncated node record at 0x{off:X}")
        name = rec[:NODE_NAME_LEN].rstrip(b"\x00")
        fields = struct.unpack_from("<6I", rec, NODE_NAME_LEN)
        return XtcNode(off, name.decode("latin1"), fields)

    def _parse_header(self):
        self._fh.seek(0)
        hdr = self._fh.read(NODE_RECORD_LEN)
        if len(hdr) < NODE_RECORD_LEN or hdr[:16] != MAGIC:
            raise XtcError(f"{self.path}: not a MOS DATAFILE2.0 file")
        self.root = self._read_node(0)
        self.node_start = self.root.next_sibling
        if self.node_start < NODE_RECORD_LEN or self.node_start >= self.size:
            raise XtcError(f"{self.path}: implausible node start "
                           f"0x{self.node_start:X}")

    def _parse_nodes(self):
        self.nodes: list[XtcNode] = []
        off = self.node_start
        while off + NODE_RECORD_LEN <= self.size:
            self._fh.seek(off)
            name = self._fh.read(NODE_NAME_LEN).rstrip(b"\x00")
            if not NAME_RE.match(name):
                break
            self.nodes.append(self._read_node(off))
            off += NODE_RECORD_LEN
        self.textures = self.node(TEXTURES)
        self.image_dir = self.node(IMAGEDIRECTORY)
        if self.textures is None or self.image_dir is None:
            seen = [n.name for n in self.nodes]
            sample = ", ".join(seen[:8]) + (" ..." if len(seen) > 8 else "")
            raise XtcError(
                f"{self.path}: unsupported XTC variant "
                f"({len(seen)} nodes: {sample}); expected the flat "
                f"{TEXTURES} + {IMAGEDIRECTORY} layout")

    def node(self, name: str):
        for n in self.nodes:
            if n.name == name:
                return n
        return None

    # -- image directory ------------------------------------------------
    def _parse_directory(self):
        d = self.image_dir
        if d.data_offset + d.data_size > self.size:
            raise XtcError(f"{self.path}: {IMAGEDIRECTORY} payload "
                           f"0x{d.data_offset:X}+0x{d.data_size:X} past EOF")
        self._fh.seek(d.data_offset)
        payload = self._fh.read(d.data_size)
        blob_off = self.textures.data_offset
        blob_end = blob_off + self.textures.data_size
        self._blob = (blob_off, blob_end)

        self.images: list[XtcImage] = []
        off = 0
        while off < len(payload):
            if payload[off:off + 4] != b"\xff\xff\xff\xff":
                raise XtcError(f"{self.path}: lost framing at directory "
                               f"+0x{off:X} ({payload[off:off+8]!r})")
            name_len = _u32(payload, off + 8)
            if name_len == 0 or name_len > 60:
                raise XtcError(f"{self.path}: implausible name length "
                               f"{name_len} at directory +0x{off:X}")
            name = payload[off + 12:off + 12 + name_len]
            try:
                name_text = name.decode("ascii")
            except UnicodeDecodeError as e:
                raise XtcError(f"{self.path}: non-ASCII image name at "
                               f"directory +0x{off:X}: {name!r}") from e
            pad = (-name_len) % 4
            desc_off = off + 12 + name_len + pad
            nxt = payload.find(IMAGE_MARKER, desc_off)
            if nxt < 0:
                nxt = len(payload)
            desc_size = nxt - desc_off
            if desc_size < DESC_MIN_BYTES:
                raise XtcError(f"{self.path}: image {name_text!r} descriptor "
                               f"only {desc_size} bytes")
            desc = list(struct.unpack_from("<%dI" % DESC_U32, payload, desc_off))
            self.images.append(XtcImage(name_text, off, nxt - off, desc_off, desc))
            off = nxt

        # stored_size: each record's block runs to the next record's block.
        for i, img in enumerate(self.images):
            if i + 1 < len(self.images):
                end = self.images[i + 1].data_offset
            else:
                end = blob_end
            img.stored_size = max(0, end - img.data_offset)
        # (a handful of records carry trailing data past desc[12]; note it)
        if sum(img.record_size for img in self.images) != len(payload):
            self.notes.append("image records do not tile the directory payload")

    # -- queries --------------------------------------------------------
    def image(self, name: str):
        for img in self.images:
            if img.name == name:
                return img
        return None

    def names(self) -> list[str]:
        return [img.name for img in self.images]

    def blob_region(self):
        return self._blob

    # -- mip chain / decode ---------------------------------------------
    def read_mips(self, img: XtcImage) -> list[XtcMip]:
        """Decode an image's packed-block mip chain (raises ``XtcError``).

        The block is ``u32 mip_offset[n]`` (absolute) followed by one
        ``[0x2C | 0x1C]-byte header + data`` record per mip.  ``n`` is
        recovered from the first table entry.
        """
        hdrlen = MIP_HEADER_COMPRESSED if img.compressed else MIP_HEADER_RAW
        self._fh.seek(img.data_offset)
        first = self._fh.read(4)
        if len(first) < 4:
            raise XtcError(f"{img.name}: truncated mip table")
        offs0 = _u32(first, 0)
        rel = offs0 - img.data_offset
        if rel <= 0 or rel % 4 or not (4 <= rel <= 4 * 32):
            raise XtcError(f"{img.name}: implausible mip table "
                           f"(first offset 0x{offs0:X}, block at "
                           f"0x{img.data_offset:X})")
        n = rel // 4
        self._fh.seek(img.data_offset)
        table = list(struct.unpack_from("<%dI" % n, self._fh.read(4 * n), 0))
        block_end = img.data_offset + img.stored_size
        mips: list[XtcMip] = []
        for i, off in enumerate(table):
            if off + hdrlen > self.size or off < img.data_offset:
                raise XtcError(f"{img.name}: mip {i} header 0x{off:X} outside "
                               f"the file/block")
            self._fh.seek(off)
            hdr_bytes = self._fh.read(hdrlen)
            if len(hdr_bytes) < hdrlen:
                raise XtcError(f"{img.name}: truncated mip {i} header")
            header = list(struct.unpack_from(
                "<%dI" % (hdrlen // 4), hdr_bytes, 0))
            width, height = header[3], header[4]
            if img.compressed:
                size = header[8]
                codec = header[7]
            else:
                size = header[2]
                codec = CODEC_RAW
            mip = XtcMip(i, width, height, size, codec, img.compressed,
                         off + hdrlen, header)
            mips.append(mip)
            nxt = table[i + 1] if i + 1 < n else block_end
            if off + hdrlen + size != nxt:
                raise XtcError(
                    f"{img.name}: mip {i} chain break "
                    f"(0x{off + hdrlen + size:X} != 0x{nxt:X})")
            if i + 1 < n and not _is_pow2(width) and width != 1:
                raise XtcError(f"{img.name}: mip {i} non-pow2 width {width}")
        return mips

    def try_read_mips(self, img: XtcImage):
        """``(mips, None)`` on success or ``(None, reason)`` on failure."""
        try:
            return self.read_mips(img), None
        except XtcError as e:
            return None, str(e)

    def decode(self, img: XtcImage, level: int = 0):
        """Decode one mip level to ``(width, height, rgba_bytes)``.

        Only the S3TC codecs (DXT1/BC1, DXT5/BC3) are decoded; the raw 32-bit
        linear images are returned in file byte order (channel order is *not*
        claimed -- see the module docstring)."""
        mips = self.read_mips(img)
        if not (0 <= level < len(mips)):
            raise XtcError(f"{img.name}: mip {level} out of range "
                           f"(0..{len(mips) - 1})")
        mip = mips[level]
        self._fh.seek(mip.data_offset)
        data = self._fh.read(mip.size)
        if len(data) < mip.size:
            raise XtcError(f"{img.name}: truncated mip {level} data")
        if not mip.compressed:
            return mip.width, mip.height, data
        if mip.codec == CODEC_DXT1:
            return mip.width, mip.height, bc1_decode(
                data, mip.width, mip.height)
        if mip.codec == CODEC_DXT3:
            return mip.width, mip.height, bc2_decode(
                data, mip.width, mip.height)
        if mip.codec == CODEC_DXT5:
            return mip.width, mip.height, bc3_decode(
                data, mip.width, mip.height)
        raise XtcError(f"{img.name}: mip {level} unknown codec "
                       f"{mip.codec} ({mip.size} bytes)")

    # -- validation -----------------------------------------------------
    def validate(self) -> list[str]:
        problems: list[str] = []
        if self.root.name != MAGIC.rstrip(b"\x00").decode("latin1"):
            problems.append("root name is not the MOS magic")
        if self.node_start < NODE_RECORD_LEN:
            problems.append("node_start inside the header")
        # node payloads must tile [0x30, node_start)
        regions = sorted((n.data_offset, n.data_size) for n in self.nodes)
        expect = NODE_RECORD_LEN
        for off, size in regions:
            if off != expect:
                problems.append(f"node payload gap/overlap at 0x{off:X} "
                                f"(expected 0x{expect:X})")
            expect = off + size
        if expect != self.node_start:
            problems.append(f"node payloads end at 0x{expect:X}, "
                            f"node table starts at 0x{self.node_start:X}")
        if self.image_dir.data_offset != self.textures.data_offset + \
                self.textures.data_size:
            problems.append("IMAGEDIRECTORY5 does not follow TEXTURES")
        # directory records
        total = 0
        prev = None
        blob_off, blob_end = self._blob
        for i, img in enumerate(self.images):
            total += img.record_size
            if img.desc[2] != 0x300:
                problems.append(f"{img.name}: desc[2] != 0x300")
            if not _is_pow2(img.width) or not _is_pow2(img.height):
                problems.append(f"{img.name}: non-power-of-two "
                                f"{img.width}x{img.height}")
            exp_mips = int(math.log2(max(img.width, img.height))) + 1
            if img.mip_levels != exp_mips:
                problems.append(f"{img.name}: mip count {img.mip_levels} != "
                                f"log2(max(w,h))+1 ({exp_mips})")
            if not (blob_off <= img.data_offset < blob_end):
                problems.append(f"{img.name}: data offset "
                                f"0x{img.data_offset:X} outside TEXTURES blob "
                                f"[0x{blob_off:X},0x{blob_end:X})")
            if prev is not None and img.data_offset <= prev:
                problems.append(f"{img.name}: data offset not increasing "
                                f"({img.data_offset} <= {prev})")
            prev = img.data_offset
            if img.stored_size and img.data_offset + img.stored_size > blob_end:
                problems.append(f"{img.name}: stored block runs past blob")
        if total != self.image_dir.data_size:
            problems.append(f"records tile {total} bytes, directory is "
                            f"{self.image_dir.data_size}")
        return problems

    def summary(self) -> dict:
        blob_off, blob_end = self._blob
        mips = {}
        for img in self.images:
            mips[img.mip_levels] = mips.get(img.mip_levels, 0) + 1
        chain_ok = 0
        codecs: dict[str, int] = {}
        for img in self.images:
            got, _reason = self.try_read_mips(img)
            if got is None:
                continue
            chain_ok += 1
            for m in got:
                codecs[m.codec_name()] = codecs.get(m.codec_name(), 0) + 1
        return {
            "path": self.path,
            "size": self.size,
            "node_start": self.node_start,
            "nodes": [n.as_dict() for n in self.nodes],
            "image_count": len(self.images),
            "images_declared_count": self.image_dir.count,
            "color_images": sum(1 for i in self.images if i.color),
            "compressed_images": sum(1 for i in self.images if i.compressed),
            "blob_offset": blob_off,
            "blob_size": blob_end - blob_off,
            "dimensions": sorted({(i.width, i.height) for i in self.images}),
            "mip_level_histogram": mips,
            "mip_chain_ok_images": chain_ok,
            "mip_chain_bad_images": len(self.images) - chain_ok,
            "mip_codec_histogram": codecs,
            "notes": self.notes,
        }


def report_file(path: str, want_json: bool, do_list: bool,
                do_find: str | None, extract: str | None,
                out: str | None, do_mips: str | None = None,
                decode: str | None = None, decode_mip: int = 0) -> dict:
    with Xtc(path) as x:
        rec = x.summary()
        rec["problems"] = x.validate()
        if do_find is not None:
            img = x.image(do_find)
            rec["find"] = img.as_dict() if img else None
        if do_list:
            rec["images"] = [img.as_dict() for img in x.images]
        if do_mips is not None:
            img = x.image(do_mips)
            if img is None:
                raise XtcError(f"no image named {do_mips!r}")
            mips, reason = x.try_read_mips(img)
            rec["mips"] = {
                "name": img.name,
                "mip_count": len(mips) if mips else 0,
                "declared_mip_levels": img.mip_levels,
                "reason": reason,
                "levels": [m.as_dict() for m in (mips or [])],
            }
        if extract is not None:
            img = x.image(extract)
            if img is None:
                raise XtcError(f"no image named {extract!r}")
            if not out:
                raise XtcError("--extract needs --out FILE")
            x._fh.seek(img.data_offset)
            blob = x._fh.read(img.stored_size)
            with open(out, "wb") as f:
                f.write(blob)
            rec["extracted"] = {"name": img.name, "out": out,
                                "bytes": len(blob),
                                "data_offset": img.data_offset}
        if decode is not None:
            img = x.image(decode)
            if img is None:
                raise XtcError(f"no image named {decode!r}")
            if not out:
                raise XtcError("--decode needs --out FILE.png")
            w, h, rgba = x.decode(img, decode_mip)
            write_png(out, w, h, rgba)
            mips, _r = x.try_read_mips(img)
            codec = mips[decode_mip].codec_name() if mips else "?"
            rec["decoded"] = {"name": img.name, "out": out, "mip": decode_mip,
                              "width": w, "height": h, "codec": codec}

    if not want_json:
        print(f"== {os.path.basename(path)}  ({rec['size']} bytes, "
              f"{rec['image_count']} images, "
              f"blob 0x{rec['blob_offset']:X}..0x"
              f"{rec['blob_offset'] + rec['blob_size']:X})")
        for n in rec["nodes"]:
            print(f"   node {n['name']:<16} @0x{n['offset']:X} "
                  f"payload 0x{n['data_offset']:X}+0x{n['data_size']:X} "
                  f"count={n['count']}")
        print(f"   color={rec['color_images']} "
              f"compressed={rec['compressed_images']} "
              f"declared_count={rec['images_declared_count']}")
        print(f"   dimensions={rec['dimensions']}")
        print(f"   mip chains: ok={rec['mip_chain_ok_images']} "
              f"unparsed={rec['mip_chain_bad_images']} "
              f"codecs={rec['mip_codec_histogram']}")
        if rec.get("find") is not None:
            i = rec["find"]
            print(f"   {i['name']}: {i['width']}x{i['height']} "
                  f"mips={i['mip_levels']} color={i['color']} "
                  f"compressed={i['compressed']} "
                  f"base_mip={i['base_mip_bytes']}B "
                  f"block@0x{i['data_offset']:X}+{i['stored_size']}B")
        elif do_find is not None:
            print(f"   {do_find!r}: NOT FOUND")
        if do_list:
            for i in rec.get("images", []):
                print(f"   {i['name']:<28} {i['width']:>4}x{i['height']:<4} "
                      f"mips={i['mip_levels']:<2} "
                      f"{'DXT' if i['compressed'] else 'raw'} "
                      f"base={i['base_mip_bytes']:>8}B "
                      f"@0x{i['data_offset']:08X}+{i['stored_size']}")
        if rec.get("extracted"):
            e = rec["extracted"]
            print(f"   extracted {e['name']} -> {e['out']} ({e['bytes']} bytes)")
        if rec.get("mips") is not None:
            m = rec["mips"]
            print(f"   mips of {m['name']}: {m['mip_count']} parsed "
                  f"(declared {m['declared_mip_levels']})")
            if m["reason"]:
                print(f"     problem: {m['reason']}")
            for lv in m["levels"]:
                print(f"     mip{lv['level']:<2} {lv['width']:>5}x{lv['height']:<5} "
                      f"{lv['codec_name']:<5} {lv['size']:>9}B "
                      f"@0x{lv['data_offset']:X}"
                      f"{' (empty)' if lv['empty'] else ''}")
        if rec.get("decoded"):
            d = rec["decoded"]
            print(f"   decoded {d['name']} mip{d['mip']} {d['codec']} "
                  f"{d['width']}x{d['height']} -> {d['out']}")
        if rec["problems"]:
            for p in rec["problems"]:
                print(f"   PROBLEM: {p}")
        else:
            print("   structural check: ok")
    return rec


def check_files(paths: list[str]) -> int:
    rc = 0
    for p in paths:
        try:
            with Xtc(p) as x:
                problems = x.validate()
                n = len(x.images)
                notes = list(x.notes)
        except XtcError as e:
            print(f"FAIL {os.path.basename(p)}: {e}")
            rc = 1
            continue
        if problems:
            rc = 1
            print(f"FAIL {os.path.basename(p)}: {'; '.join(problems)}")
        else:
            print(f"ok   {os.path.basename(p)}: {n} images")
        for note in notes:
            print(f"     note: {note}")
    return rc


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("path", nargs="?", help="a .xtc texture pack")
    ap.add_argument("--check", action="store_true",
                    help="validate container + directory structure "
                         "(exit 1 on problems)")
    ap.add_argument("--list", action="store_true", help="list every image")
    ap.add_argument("--find", metavar="NAME", help="print one image entry")
    ap.add_argument("--extract", metavar="NAME",
                    help="write an image's packed block to --out FILE")
    ap.add_argument("--out", metavar="FILE", help="output for --extract/--decode")
    ap.add_argument("--mips", metavar="NAME",
                    help="print an image's parsed mip chain")
    ap.add_argument("--decode", metavar="NAME",
                    help="decode an image's mip to an RGBA PNG at --out FILE")
    ap.add_argument("--mip", type=int, default=0, metavar="N",
                    help="mip level for --decode (default 0)")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)

    if args.check:
        if not args.path:
            ap.error("--check needs a .xtc path")
        return check_files([args.path])

    if not args.path:
        ap.error("provide a .xtc path (or use --check)")
    rec = report_file(args.path, args.json, args.list, args.find,
                      args.extract, args.out, args.mips, args.decode, args.mip)
    if args.json:
        print(json.dumps(rec, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
