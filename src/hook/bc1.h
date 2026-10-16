#pragma once

// GH #61: minimal BC1 / DXT1 (S3TC) block decoder.
//
// The shipped Riddick skin diffuse (``Riddick_ArmScar_D``) is stored as DXT1 in
// ``Content/Models.xtc``; ``tools/xtc_bake.py`` bakes those blocks verbatim into
// ``native_hand_texture_data.h`` and this header turns them into RGBA8 for a
// fixed-function ``GL_TEXTURE_2D`` upload. The maths mirrors
// ``tools/mos_xtc.py`` (``_rgb565`` / ``_bc1_base`` / ``bc1_decode``) exactly --
// integer 5/6/5 expansion and integer palette interpolation -- so the Python
// reference samples baked into the asset are a real cross-check, not an
// approximation. Header-only and dependency-free so it can be unit-tested
// headlessly (``tests/native_hand_texture_tests.cpp``).

#include <cstdint>

namespace RiddickVR {

// Blocks needed for a ``width x height`` DXT1 surface.
inline unsigned int Bc1SurfaceBytes(unsigned int width, unsigned int height) {
    const unsigned int bw = (width + 3u) / 4u;
    const unsigned int bh = (height + 3u) / 4u;
    return bw * bh * 8u;
}

// Expand one RGB565 colour to 8-bit RGB (5/6/5, integer floor).
inline void Bc1Rgb565ToRgb(unsigned int color, unsigned char* rgb) {
    rgb[0] = static_cast<unsigned char>((((color >> 11) & 0x1Fu) * 255u) / 31u);
    rgb[1] = static_cast<unsigned char>((((color >> 5) & 0x3Fu) * 255u) / 63u);
    rgb[2] = static_cast<unsigned char>(((color & 0x1Fu) * 255u) / 31u);
}

// Decode one 8-byte BC1 block to 16 RGBA8 texels, row-major (4x4). ``out`` must
// hold 64 bytes.
inline void Bc1DecodeBlock(const unsigned char* block, unsigned char* out) {
    const unsigned int c0 = static_cast<unsigned int>(block[0]) |
                            (static_cast<unsigned int>(block[1]) << 8);
    const unsigned int c1 = static_cast<unsigned int>(block[2]) |
                            (static_cast<unsigned int>(block[3]) << 8);

    unsigned char palette[4][3];
    Bc1Rgb565ToRgb(c0, palette[0]);
    Bc1Rgb565ToRgb(c1, palette[1]);
    if (c0 > c1) {
        for (int k = 0; k < 3; ++k) {
            palette[2][k] = static_cast<unsigned char>(
                (2u * palette[0][k] + palette[1][k]) / 3u);
            palette[3][k] = static_cast<unsigned char>(
                (palette[0][k] + 2u * palette[1][k]) / 3u);
        }
    } else {
        for (int k = 0; k < 3; ++k) {
            palette[2][k] = static_cast<unsigned char>(
                (static_cast<unsigned int>(palette[0][k]) +
                 static_cast<unsigned int>(palette[1][k])) / 2u);
        }
        palette[3][0] = palette[3][1] = palette[3][2] = 0;
    }

    const unsigned int bits = static_cast<unsigned int>(block[4]) |
                              (static_cast<unsigned int>(block[5]) << 8) |
                              (static_cast<unsigned int>(block[6]) << 16) |
                              (static_cast<unsigned int>(block[7]) << 24);
    for (int i = 0; i < 16; ++i) {
        const unsigned int index = (bits >> (2 * i)) & 3u;
        out[i * 4 + 0] = palette[index][0];
        out[i * 4 + 1] = palette[index][1];
        out[i * 4 + 2] = palette[index][2];
        out[i * 4 + 3] = 255;
    }
}

// Decode a whole BC1 surface to top-down RGBA8 (``width * height * 4`` bytes).
// Non-multiple-of-4 edges are decoded then clipped (matching the Python
// decoder). ``rgba`` may be null, in which case nothing is written.
inline void Bc1DecodeSurface(const unsigned char* blocks, unsigned int width,
                             unsigned int height, unsigned char* rgba) {
    if (!blocks || !rgba || width == 0 || height == 0) {
        return;
    }
    const unsigned int bw = (width + 3u) / 4u;
    const unsigned int bh = (height + 3u) / 4u;
    for (unsigned int by = 0; by < bh; ++by) {
        for (unsigned int bx = 0; bx < bw; ++bx) {
            unsigned char texels[64];
            Bc1DecodeBlock(blocks + (by * bw + bx) * 8u, texels);
            for (unsigned int py = 0; py < 4; ++py) {
                for (unsigned int px = 0; px < 4; ++px) {
                    const unsigned int x = bx * 4u + px;
                    const unsigned int y = by * 4u + py;
                    if (x >= width || y >= height) {
                        continue;
                    }
                    const unsigned char* src = &texels[(py * 4u + px) * 4u];
                    unsigned char* dst = &rgba[(y * width + x) * 4u];
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                    dst[3] = src[3];
                }
            }
        }
    }
}

} // namespace RiddickVR
