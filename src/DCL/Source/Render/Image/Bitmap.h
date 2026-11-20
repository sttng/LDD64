// LEGO Digital Designer — restored source
// LEGO::Bitmap — caller-owned pixels that codecs decode into / encode from and texture factories
// upload; BitmapFormat — the pixel-format codes. Header-only, non-virtual, sizeof 0x20. RTTI via
// BasicArray<Bitmap> / BasicArray<BasicArray<Bitmap>> (mip / cube lists, Mac vtables 0x760e28 /
// 0x760e10). Directory inferred: the image layer is one Win block (0x6985d0-0x6a2dbf) before
// zlib/libpng/libjpeg, apart from Base (0x401000..) and the OpenGL renderer (0x4ec000..).
#pragma once
#include "LEGO/Types.h"
#include "Color.h"

namespace LEGO {

// Pixel-format codes (names inferred; the values are the ones the code compares against).
//   bits  0- 7  bits per pixel (1, 2, 4, 8, 16, 24, 32)
//   bits  8-11  channel layout of the packed formats (see BitmapPixelFormat in
//               BitmapCodecPixelConverter.cpp for the byte/bit positions)
//   bit  30     has alpha
//   bit  31     indexed (palette in Bitmap::m_palette, 1 << bpp entries)
// Byte order names below are the order in memory (little-endian words).
enum BitmapFormat {
    kBitmapFormatIndexedFlag = 0x80000000,
    kBitmapFormatAlphaFlag   = 0x40000000,

    kBitmapFormatIndexed1    = 0x80000101,
    kBitmapFormatIndexed2    = 0x80000102,
    kBitmapFormatIndexed4    = 0x80000104,
    kBitmapFormatIndexed8    = 0x80000108,

    kBitmapFormatRGB565      = 0x110,          // 0xf800 / 0x07e0 / 0x001f
    kBitmapFormatXRGB1555    = 0x210,          // 0x7c00 / 0x03e0 / 0x001f
    kBitmapFormatARGB1555    = 0x40000310,     // + 0x8000
    kBitmapFormatXRGB4444    = 0x410,          // masks not reported (BitmapPixelFormat: 0)
    kBitmapFormatARGB4444    = 0x40000510,     // 0x0f00 / 0x00f0 / 0x000f / 0xf000

    kBitmapFormatBGR24       = 0x118,          // bytes B, G, R (Windows DIB order)
    kBitmapFormatRGB24       = 0x218,          // bytes R, G, B
    kBitmapFormatBGRX32      = 0x120,          // bytes B, G, R, x
    kBitmapFormatBGRA32      = 0x40000220,     // bytes B, G, R, A
    kBitmapFormatXBGR32      = 0x320,          // bytes x, B, G, R
    kBitmapFormatABGR32      = 0x40000420,     // bytes A, B, G, R
    kBitmapFormatXRGB32      = 0x520,          // bytes x, R, G, B
    kBitmapFormatARGB32      = 0x40000620,     // bytes A, R, G, B
    kBitmapFormatRGBX32      = 0x720,          // bytes R, G, B, x
    kBitmapFormatRGBA32      = 0x40000820      // bytes R, G, B, A
};

// sizeof 0x20 (Mac and Win). Plain data; the pixels are owned by whoever filled it in.
// Field names inferred.
class Bitmap {
public:
    Bitmap()
        : m_width(0), m_height(0), m_depth(0), m_format(0), m_dataSize(0), m_pitch(0),
          m_data(0), m_palette(0)
    {
    }

    static bool IsIndexed(UInt32 format) { return (format & kBitmapFormatIndexedFlag) != 0; }
    static bool HasAlpha(UInt32 format)  { return (format & kBitmapFormatAlphaFlag) != 0; }
    static UInt32 GetBitsPerPixel(UInt32 format) { return format & 0xff; }

    // Exchanges the red and blue channel positions of a byte-ordered format; everything else
    // is returned unchanged. Win 0x4f5310 (emitted with the OpenGL texture code; the PNG and
    // JPG codecs use it to get 0x218 from 0x118). Mac: no caller in the image layer. Covers
    // the float formats of the texture code too (0x130/0x230, 0x160/0x260, 0x40000440/0x840,
    // 0x40000480/0x880). (name inferred)
    static UInt32 SwapRedBlueFormat(UInt32 format)
    {
        switch (format) {
        case 0x118:      return 0x218;
        case 0x218:      return 0x118;
        case 0x120:      return 0x520;
        case 0x520:      return 0x120;
        case 0x130:      return 0x230;
        case 0x230:      return 0x130;
        case 0x160:      return 0x260;
        case 0x260:      return 0x160;
        case 0x320:      return 0x720;
        case 0x720:      return 0x320;
        case 0x40000220: return 0x40000620;
        case 0x40000620: return 0x40000220;
        case 0x40000420: return 0x40000820;
        case 0x40000820: return 0x40000420;
        case 0x40000440: return 0x40000840;
        case 0x40000840: return 0x40000440;
        case 0x40000480: return 0x40000880;
        case 0x40000880: return 0x40000480;
        default:         return format;
        }
    }

    UInt32 m_width;     // +0x00
    UInt32 m_height;    // +0x04
    UInt32 m_depth;     // +0x08 (3D textures; unused by the codecs)
    UInt32 m_format;    // +0x0c BitmapFormat
    UInt32 m_dataSize;  // +0x10 bytes at m_data (pitch * height; informational for the codecs)
    Int32  m_pitch;     // +0x14 bytes per row (may be negative for bottom-up rows)
    void*  m_data;      // +0x18
    Color* m_palette;   // +0x1c 1 << bpp entries for indexed formats (decode copies into it)
};

} // namespace LEGO
