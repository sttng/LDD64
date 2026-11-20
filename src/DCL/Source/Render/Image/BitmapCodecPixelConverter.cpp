// LEGO Digital Designer — restored source
// TU 300 (Mac 0x2aac30-0x2af5df), Win 0x69f9b0-0x6a2cff. See BitmapCodecPixelConverter.h.
//
// Conversion routines by (source bpp, destination bpp):
//   from \ to   1        2        4        8        16       24       32
//   1         2ab400   2ab500   2ab5c0   2ab650   2ab710   2ab800   2ab8c0     (Mac)
//             69ffb0   6a0010   6a00b0   6a0130   6a0fb0   6a1040   6a10e0     (Win)
//   4                           2ab9d0   2abac0   2abb80   2abc40   2abd10
//                               6a0190   6a01e0   6a1170   6a1210   6a12c0
//   8                                    2abdd0   2abed0   2ac040   2ac160
//                                        6a0250   6a1350   6a1400   6a14b0
//   16                                            2ac2c0   2ac9d0   2acab0
//                                                 6a02a0   6a0570   6a0660
//   24                                            2acd70   2ad520   2ad730
//                                                 6a1650   6a1540   6a0860
//   32                                            2ad920   2ae880   2ae9e0
//                                                 6a1d80   6a09d0   6a0a60
// Palette lookup routines: 16-bit Mac 0x2aef60 / Win 0x6a0e30, 24/32-bit Mac 0x2af380 / Win
// 0x6a0cd0 (table / source / destination in registers in both builds).
//
// Float -> integer conversions truncate in both builds (Mac cvttss2si, Win fistp with the
// rounding control temporarily set to chop); the products differ in precision (see ToByte).
#include "BitmapCodecPixelConverter.h"
#include <string.h>

namespace LEGO {

namespace {

// n-bit -> 8-bit expansion tables (Mac 0x63b780.., Win 0x9de578..).
const UInt32 kExpand0[1]  = { 0 };                                       // Mac 0x63b780, Win 0x9de578
const UInt32 kExpand1[2]  = { 0, 255 };                                  // Mac 0x63b784, Win 0x9de57c
const UInt32 kExpand4[16] = {                                            // Mac 0x63b78c, Win 0x9de588
    0, 17, 34, 51, 68, 85, 102, 119, 136, 153, 170, 187, 204, 221, 238, 255 };
const UInt32 kExpand5[32] = {                                            // Mac 0x63b7cc, Win 0x9de5c8
    0, 8, 16, 24, 32, 41, 49, 57, 65, 74, 82, 90, 98, 106, 115, 123,
    131, 139, 148, 156, 164, 172, 180, 189, 197, 205, 213, 222, 230, 238, 246, 255 };
const UInt32 kExpand6[64] = {                                            // Mac 0x63b84c, Win 0x9de648
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    64, 68, 72, 76, 80, 85, 89, 93, 97, 101, 105, 109, 113, 117, 121, 125,
    129, 133, 137, 141, 145, 149, 153, 157, 161, 165, 170, 174, 178, 182, 186, 190,
    194, 198, 202, 206, 210, 214, 218, 222, 226, 230, 234, 238, 242, 246, 250, 255 };

typedef BitmapPixelSource      Src;
typedef BitmapPixelDestination Dst;

enum { R = 0, G = 1, B = 2, A = 3 };

inline bool HasAlpha(const BitmapPixelFormat& f) { return (f.m_format & kBitmapFormatAlphaFlag) != 0; }

// component * 255, truncated. Platform difference: the Mac multiplies in single precision
// (mulss 255.0f, cvttss2si), the Win build on the x87 with the double constant 255.0 (exact
// product, chopped by fistp). Values just below k/255 (e.g. Color::FromBytes results) can
// differ by one.
inline Int32 ToByte(Real32 c)
{
#if defined(_WIN32)
    return (Int32)((Real64)c * 255.0);
#else
    return (Int32)(c * 255.0f);
#endif
}

inline const UInt8* SrcRow(const Src* s) { return static_cast<const UInt8*>(s->m_data); }
inline UInt8*       DstRow(const Dst* d) { return static_cast<UInt8*>(d->m_data); }

inline UInt32 Bit1(const UInt8* row, UInt32 x) { return (row[x >> 3] >> (7 - (x & 7))) & 1; }

// Colour key packed into the source's channel layout (16-bit sources: with the source losses).
inline UInt32 PackedKey16(const Src* s)
{
    const BitmapPixelFormat& f = s->m_pixelFormat;
    return ((UInt32)ToByte(s->m_colorKey.b) >> f.m_loss[B]) << f.m_shift[B] |
           ((UInt32)ToByte(s->m_colorKey.g) >> f.m_loss[G]) << f.m_shift[G] |
           ((UInt32)ToByte(s->m_colorKey.r) >> f.m_loss[R]) << f.m_shift[R];
}

// Palette -> packed 16-bit values. Mac 0x2aef60 (ECX table, EDX source, stack destination),
// Win 0x6a0e30 (stack table/source, EDI destination). An entry whose RGB equals the colour key
// gets no alpha when the destination has alpha.
void BuildPaletteTable16(UInt16* table, const Src* src, const BitmapPixelFormat& dst)
{
    bool useKey = src->m_useColorKey && HasAlpha(dst);
    UInt32 count = 1u << (src->m_pixelFormat.m_format & 0x1f);
    const Color* palette = src->m_pixelFormat.m_palette;
    for (UInt32 i = 0; i < count; ++i) {
        const Color& c = palette[i];
        UInt16 v = (UInt16)(((UInt16)ToByte(c.b) >> dst.m_loss[B]) << dst.m_shift[B] |
                            ((UInt16)ToByte(c.g) >> dst.m_loss[G]) << dst.m_shift[G] |
                            ((UInt16)ToByte(c.r) >> dst.m_loss[R]) << dst.m_shift[R]);
        if (!useKey || !c.EqualsRGB(src->m_colorKey))
            v = (UInt16)(v | ((UInt16)ToByte(c.a) >> dst.m_loss[A]) << dst.m_shift[A]);
        table[i] = v;
    }
}

// Palette -> packed 24/32-bit values. Mac 0x2af380, Win 0x6a0cd0 (EBX destination).
void BuildPaletteTable32(UInt32* table, const Src* src, const BitmapPixelFormat& dst)
{
    bool useKey = src->m_useColorKey && HasAlpha(dst);
    UInt32 count = 1u << (src->m_pixelFormat.m_format & 0x1f);
    const Color* palette = src->m_pixelFormat.m_palette;
    for (UInt32 i = 0; i < count; ++i) {
        const Color& c = palette[i];
        UInt32 v = (UInt32)ToByte(c.b) << dst.m_shift[B] |
                   (UInt32)ToByte(c.g) << dst.m_shift[G] |
                   (UInt32)ToByte(c.r) << dst.m_shift[R];
        if (!useKey || !c.EqualsRGB(src->m_colorKey))
            v |= (UInt32)ToByte(c.a) << dst.m_shift[A];
        table[i] = v;
    }
}

// Row copy of the identical indexed layouts: Mac 0x2ab400 (1->1), 0x2ab9d0 (4->4),
// 0x2abdd0 (8->8); Win 0x69ffb0, 0x6a0190, 0x6a0250.
void CopyRows(UInt32 rowBytes, UInt32 height, const Src* src, const Dst* dst)
{
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    for (; height != 0; --height) {
        memcpy(d, s, rowBytes);
        s += src->m_pitch;
        d += dst->m_pitch;
    }
}

// --- 1-bit source ----------------------------------------------------------------------------

void Convert1To1(UInt32 width, UInt32 height, const Src* src, const Dst* dst)   // Mac 0x2ab400, Win 0x69ffb0
{
    CopyRows((width + 7) >> 3, height, src, dst);
}

// NOTE: 2-bit pixels are filled from the top bits down, each byte read-modify-written.
void Convert1To2(UInt32 width, UInt32 height, const Src* src, const Dst* dst)   // Mac 0x2ab500, Win 0x6a0010
{
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        for (UInt32 x = 0; x < width; ++x) {
            UInt32 shift = (x * 2) & 6;
            d[x >> 2] = (UInt8)(((0x3f3f >> shift) & d[x >> 2]) | (Bit1(s, x) << (6 - shift)));
        }
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

void Convert1To4(UInt32 width, UInt32 height, const Src* src, const Dst* dst)   // Mac 0x2ab5c0, Win 0x6a00b0
{
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        for (UInt32 x = 0; x < width; ++x) {
            UInt8 bit = (UInt8)Bit1(s, x);
            if ((x & 1) == 0)
                d[x >> 1] = (UInt8)((d[x >> 1] & 0x0f) | (bit << 4));
            else
                d[x >> 1] = (UInt8)((d[x >> 1] & 0xf0) | bit);
        }
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

void Convert1To8(UInt32 width, UInt32 height, const Src* src, const Dst* dst)   // Mac 0x2ab650, Win 0x6a0130
{
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        for (UInt32 x = 0; x < width; ++x)
            d[x] = (UInt8)Bit1(s, x);
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

void Convert1To16(UInt32 width, UInt32 height, const Src* src, const Dst* dst)  // Mac 0x2ab710, Win 0x6a0fb0
{
    UInt16 table[2];
    BuildPaletteTable16(table, src, dst->m_pixelFormat);
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        UInt16* p = reinterpret_cast<UInt16*>(d);
        for (UInt32 x = 0; x < width; ++x)
            p[x] = table[Bit1(s, x)];
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

inline void Store24(UInt8* p, UInt32 v)
{
    p[0] = (UInt8)v;
    p[1] = (UInt8)(v >> 8);
    p[2] = (UInt8)(v >> 16);
}

void Convert1To24(UInt32 width, UInt32 height, const Src* src, const Dst* dst)  // Mac 0x2ab800, Win 0x6a1040
{
    UInt32 table[2];
    BuildPaletteTable32(table, src, dst->m_pixelFormat);
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        for (UInt32 x = 0; x < width; ++x)
            Store24(d + x * 3, table[Bit1(s, x)]);
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

void Convert1To32(UInt32 width, UInt32 height, const Src* src, const Dst* dst)  // Mac 0x2ab8c0, Win 0x6a10e0
{
    UInt32 table[2];
    BuildPaletteTable32(table, src, dst->m_pixelFormat);
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        UInt32* p = reinterpret_cast<UInt32*>(d);
        for (UInt32 x = 0; x < width; ++x)
            p[x] = table[Bit1(s, x)];
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

// --- 4-bit source (high nibble first) ----------------------------------------------------------

inline UInt32 Nibble(const UInt8* row, UInt32 x)
{
    return (x & 1) == 0 ? (UInt32)(row[x >> 1] >> 4) : (UInt32)(row[x >> 1] & 0xf);
}

void Convert4To4(UInt32 width, UInt32 height, const Src* src, const Dst* dst)   // Mac 0x2ab9d0, Win 0x6a0190
{
    CopyRows((width + 1) >> 1, height, src, dst);
}

void Convert4To8(UInt32 width, UInt32 height, const Src* src, const Dst* dst)   // Mac 0x2abac0, Win 0x6a01e0
{
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        for (UInt32 x = 0; x < width; ++x)
            d[x] = (UInt8)Nibble(s, x);
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

void Convert4To16(UInt32 width, UInt32 height, const Src* src, const Dst* dst)  // Mac 0x2abb80, Win 0x6a1170
{
    UInt16 table[16];
    BuildPaletteTable16(table, src, dst->m_pixelFormat);
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        UInt16* p = reinterpret_cast<UInt16*>(d);
        for (UInt32 x = 0; x < width; ++x)
            p[x] = table[Nibble(s, x)];
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

void Convert4To24(UInt32 width, UInt32 height, const Src* src, const Dst* dst)  // Mac 0x2abc40, Win 0x6a1210
{
    UInt32 table[16];
    BuildPaletteTable32(table, src, dst->m_pixelFormat);
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        for (UInt32 x = 0; x < width; ++x)
            Store24(d + x * 3, table[Nibble(s, x)]);
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

void Convert4To32(UInt32 width, UInt32 height, const Src* src, const Dst* dst)  // Mac 0x2abd10, Win 0x6a12c0
{
    UInt32 table[16];
    BuildPaletteTable32(table, src, dst->m_pixelFormat);
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        UInt32* p = reinterpret_cast<UInt32*>(d);
        for (UInt32 x = 0; x < width; ++x)
            p[x] = table[Nibble(s, x)];
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

// --- 8-bit source ------------------------------------------------------------------------------

void Convert8To8(UInt32 width, UInt32 height, const Src* src, const Dst* dst)   // Mac 0x2abdd0, Win 0x6a0250
{
    CopyRows(width, height, src, dst);
}

void Convert8To16(UInt32 width, UInt32 height, const Src* src, const Dst* dst)  // Mac 0x2abed0, Win 0x6a1350
{
    UInt16 table[256];
    BuildPaletteTable16(table, src, dst->m_pixelFormat);
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        UInt16* p = reinterpret_cast<UInt16*>(d);
        for (UInt32 x = 0; x < width; ++x)
            p[x] = table[s[x]];
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

void Convert8To24(UInt32 width, UInt32 height, const Src* src, const Dst* dst)  // Mac 0x2ac040, Win 0x6a1400
{
    UInt32 table[256];
    BuildPaletteTable32(table, src, dst->m_pixelFormat);
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        for (UInt32 x = 0; x < width; ++x)
            Store24(d + x * 3, table[s[x]]);
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

void Convert8To32(UInt32 width, UInt32 height, const Src* src, const Dst* dst)  // Mac 0x2ac160, Win 0x6a14b0
{
    UInt32 table[256];
    BuildPaletteTable32(table, src, dst->m_pixelFormat);
    if (height == 0 || width == 0)
        return;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    do {
        UInt32* p = reinterpret_cast<UInt32*>(d);
        for (UInt32 x = 0; x < width; ++x)
            p[x] = table[s[x]];
        s += src->m_pitch;
        d += dst->m_pitch;
    } while (--height != 0);
}

// --- 16-bit source -----------------------------------------------------------------------------

inline UInt32 Expand(const BitmapPixelFormat& f, int c, UInt32 px)
{
    return f.m_expand[c][(px & f.m_mask[c]) >> f.m_shift[c]];
}

void Convert16To16(UInt32 width, UInt32 height, const Src* src, const Dst* dst) // Mac 0x2ac2c0, Win 0x6a02a0
{
    const BitmapPixelFormat& sf = src->m_pixelFormat;
    const BitmapPixelFormat& df = dst->m_pixelFormat;
    bool bothAlpha = HasAlpha(sf) && HasAlpha(df);
    bool useKey = src->m_useColorKey && HasAlpha(df);
    UInt16 rgbMask = (UInt16)(sf.m_mask[G] | sf.m_mask[R] | sf.m_mask[B]);
    UInt16 key = (UInt16)PackedKey16(src);
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    if (sf.m_format == df.m_format) {
        // Same layout: copy; with a key the matching pixels lose their alpha bits.
        for (; height != 0; --height) {
            if (useKey) {
                const UInt16* sp = reinterpret_cast<const UInt16*>(s);
                UInt16* dp = reinterpret_cast<UInt16*>(d);
                for (UInt32 x = 0; x < width; ++x) {
                    UInt16 masked = (UInt16)(sp[x] & rgbMask);
                    dp[x] = masked == key ? masked : sp[x];
                }
            } else {
                memcpy(d, s, width * 2);
            }
            d += dst->m_pitch;
            s += src->m_pitch;
        }
        return;
    }
    for (; height != 0; --height) {
        const UInt16* sp = reinterpret_cast<const UInt16*>(s);
        UInt16* dp = reinterpret_cast<UInt16*>(d);
        for (UInt32 x = 0; x < width; ++x) {
            UInt32 px = sp[x];
            UInt16 v = (UInt16)((Expand(sf, R, px) >> df.m_loss[R]) << df.m_shift[R] |
                                (Expand(sf, B, px) >> df.m_loss[B]) << df.m_shift[B] |
                                (Expand(sf, G, px) >> df.m_loss[G]) << df.m_shift[G]);
            if (useKey) {
                if ((UInt16)(sp[x] & rgbMask) != key)
                    v = (UInt16)(v | (Expand(sf, A, px) >> df.m_loss[A]) << df.m_shift[A]);
            } else if (bothAlpha) {
                v = (UInt16)(v | (Expand(sf, A, px) >> df.m_loss[A]) << df.m_shift[A]);
            } else {
                v = (UInt16)(v | (0xffu >> df.m_loss[A]) << df.m_shift[A]);
            }
            dp[x] = v;
        }
        s += src->m_pitch;
        d += dst->m_pitch;
    }
}

void Convert16To24(UInt32 width, UInt32 height, const Src* src, const Dst* dst) // Mac 0x2ac9d0, Win 0x6a0570
{
    const BitmapPixelFormat& sf = src->m_pixelFormat;
    const BitmapPixelFormat& df = dst->m_pixelFormat;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    for (; height != 0; --height) {
        const UInt16* sp = reinterpret_cast<const UInt16*>(s);
        for (UInt32 x = 0; x < width; ++x) {
            UInt32 px = sp[x];
            UInt32 v = Expand(sf, B, px) << df.m_shift[B] |
                       Expand(sf, G, px) << df.m_shift[G] |
                       Expand(sf, R, px) << df.m_shift[R];
            Store24(d + x * 3, v);
        }
        s += src->m_pitch;
        d += dst->m_pitch;
    }
}

void Convert16To32(UInt32 width, UInt32 height, const Src* src, const Dst* dst) // Mac 0x2acab0, Win 0x6a0660
{
    const BitmapPixelFormat& sf = src->m_pixelFormat;
    const BitmapPixelFormat& df = dst->m_pixelFormat;
    bool bothAlpha = HasAlpha(sf) && HasAlpha(df);
    bool useKey = src->m_useColorKey && HasAlpha(df);
    UInt32 rgbMask = sf.m_mask[R] | sf.m_mask[G] | sf.m_mask[B];
    UInt32 key = PackedKey16(src);
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    for (; height != 0; --height) {
        const UInt16* sp = reinterpret_cast<const UInt16*>(s);
        UInt32* dp = reinterpret_cast<UInt32*>(d);
        for (UInt32 x = 0; x < width; ++x) {
            UInt32 px = sp[x];
            UInt32 v = Expand(sf, R, px) << df.m_shift[R] |
                       Expand(sf, G, px) << df.m_shift[G] |
                       Expand(sf, B, px) << df.m_shift[B];
            if (useKey) {
                if ((px & rgbMask) != key)
                    v |= Expand(sf, A, px) << df.m_shift[A];
            } else if (bothAlpha) {
                v |= Expand(sf, A, px) << df.m_shift[A];
            } else {
                v |= 0xffu << df.m_shift[A];
            }
            dp[x] = v;
        }
        s += src->m_pitch;
        d += dst->m_pitch;
    }
}

// --- 24/32-bit sources to 16 bits: serpentine Floyd-Steinberg dithering ------------------------
// The error buffer holds `stride` ints per pixel plus one pixel of padding at each end; the
// running error terms are NOT reset between rows (both builds).
//   withAlpha: 32-bit source and destination both have alpha (alpha taken from the source)
void DitherTo16(UInt32 width, UInt32 height, const Src* src, const Dst* dst,
                UInt32 bytesPerPixel, bool withAlpha)
{
    const BitmapPixelFormat& sf = src->m_pixelFormat;
    const BitmapPixelFormat& df = dst->m_pixelFormat;
    const Int32 stride = (Int32)bytesPerPixel;                      // ints per pixel in the buffer
    UInt32 count = (width + 2) * bytesPerPixel;
    Int32* buffer = new Int32[count];
    memset(buffer, 0, count * sizeof(Int32));
    Int32* forwardStart = buffer + stride;                          // pixel slot 1
    Int32* backwardStart = buffer + width * stride;                 // pixel slot width
    UInt32 lossMask[3];
    for (int c = 0; c < 3; ++c)
        lossMask[c] = 0xffu >> (8 - df.m_loss[c]);

    Int32 errRight[3] = { 0, 0, 0 };   // 7/16 of the previous pixel's error
    Int32 accum[3] = { 0, 0, 0 };      // 5/16 of the previous + 1/16 of the one before
    UInt32 prev[3] = { 0, 0, 0 };      // previous pixel's error
    bool forward = true;
    bool useKey = src->m_useColorKey;
    UInt32 key[3] = { 0, 0, 0 };
    if (useKey) {
        key[R] = (UInt32)ToByte(src->m_colorKey.r);
        key[G] = (UInt32)ToByte(src->m_colorKey.g);
        key[B] = (UInt32)ToByte(src->m_colorKey.b);
    }

    const UInt8* srcRow = SrcRow(src);
    UInt8* dstRow = DstRow(dst);
    for (; height != 0; --height) {
        Int32* e;
        Int32 step;
        const UInt8* s;
        UInt16* d;
        Int32 sStep, dStep;
        if (forward) {
            e = forwardStart;
            step = stride;
            s = srcRow;
            d = reinterpret_cast<UInt16*>(dstRow);
            sStep = (Int32)bytesPerPixel;
            dStep = 1;
        } else {
            e = backwardStart;
            step = -stride;
            s = srcRow + (width - 1) * bytesPerPixel;
            d = reinterpret_cast<UInt16*>(dstRow) + width - 1;
            sStep = -(Int32)bytesPerPixel;
            dStep = -1;
        }
        forward = !forward;
        for (UInt32 x = 0; x < width; ++x) {
            UInt32 sr = s[sf.m_byteIndex[R]];
            UInt32 sg = s[sf.m_byteIndex[G]];
            UInt32 sb = s[sf.m_byteIndex[B]];
            if (useKey && sr == key[R] && sg == key[G] && sb == key[B]) {
                // The key colour is written exactly, without alpha, and stops the error flow.
                *d = (UInt16)((0u >> df.m_loss[A]) << df.m_shift[A] |
                              (sb >> df.m_loss[B]) << df.m_shift[B] |
                              (sg >> df.m_loss[G]) << df.m_shift[G] |
                              (sr >> df.m_loss[R]) << df.m_shift[R]);
                e[0] = 0;
                e[1] = 0;
                e[2] = 0;
                errRight[0] = errRight[1] = errRight[2] = 0;
            } else {
                Int32 v[3];
                v[R] = (Int32)sr + ((e[step + 0] + 8 + errRight[R]) >> 4);
                v[G] = (Int32)sg + ((e[step + 1] + 8 + errRight[G]) >> 4);
                v[B] = (Int32)sb + ((e[step + 2] + 8 + errRight[B]) >> 4);
                for (int c = 0; c < 3; ++c) {
                    if (v[c] > 0xfe)
                        v[c] = 0xff;
                }
                UInt32 alpha = withAlpha ? (UInt32)s[sf.m_byteIndex[A]] : 0xffu;
                *d = (UInt16)((alpha >> df.m_loss[A]) << df.m_shift[A] |
                              ((UInt32)v[B] >> df.m_loss[B]) << df.m_shift[B] |
                              ((UInt32)v[G] >> df.m_loss[G]) << df.m_shift[G] |
                              ((UInt32)v[R] >> df.m_loss[R]) << df.m_shift[R]);
                for (int c = 0; c < 3; ++c) {
                    UInt32 q = (UInt32)v[c] & lossMask[c];
                    e[c] = accum[c] + (Int32)(q * 3);
                    accum[c] = (Int32)(prev[c] + q * 5);
                    errRight[c] = (Int32)(q * 7);
                    prev[c] = q;
                }
            }
            e += step;
            s += sStep;
            d += dStep;
        }
        e[0] = accum[0];
        e[1] = accum[1];
        e[2] = accum[2];
        srcRow += src->m_pitch;
        dstRow += dst->m_pitch;
    }
    delete[] buffer;
}

// --- 24-bit source -----------------------------------------------------------------------------

void Convert24To16(UInt32 width, UInt32 height, const Src* src, const Dst* dst) // Mac 0x2acd70, Win 0x6a1650
{
    DitherTo16(width, height, src, dst, 3, false);
}

// NOTE: only the red/blue order is looked at: the destination is assumed to be 0x118 or 0x218.
void Convert24To24(UInt32 width, UInt32 height, const Src* src, const Dst* dst) // Mac 0x2ad520, Win 0x6a1540
{
    const BitmapPixelFormat& sf = src->m_pixelFormat;
    const BitmapPixelFormat& df = dst->m_pixelFormat;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    if (sf.m_format == df.m_format) {
        for (; height != 0; --height) {
            memcpy(d, s, width * 3);
            s += src->m_pitch;
            d += dst->m_pitch;
        }
        return;
    }
    UInt32 first = sf.m_byteIndex[R];
    UInt32 last = sf.m_byteIndex[B];
    if (df.m_byteIndex[B] < df.m_byteIndex[R]) {
        first = sf.m_byteIndex[B];
        last = sf.m_byteIndex[R];
    }
    UInt32 middle = sf.m_byteIndex[G];
    for (; height != 0; --height) {
        for (UInt32 x = 0; x < width; ++x) {
            d[x * 3 + 0] = s[x * 3 + first];
            d[x * 3 + 1] = s[x * 3 + middle];
            d[x * 3 + 2] = s[x * 3 + last];
        }
        s += src->m_pitch;
        d += dst->m_pitch;
    }
}

void Convert24To32(UInt32 width, UInt32 height, const Src* src, const Dst* dst) // Mac 0x2ad730, Win 0x6a0860
{
    const BitmapPixelFormat& sf = src->m_pixelFormat;
    const BitmapPixelFormat& df = dst->m_pixelFormat;
    bool useKey = src->m_useColorKey && HasAlpha(df);
    UInt32 key = (UInt32)ToByte(src->m_colorKey.b) << df.m_shift[B] |
                 (UInt32)ToByte(src->m_colorKey.g) << df.m_shift[G] |
                 (UInt32)ToByte(src->m_colorKey.r) << df.m_shift[R];
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    for (; height != 0; --height) {
        UInt32* dp = reinterpret_cast<UInt32*>(d);
        for (UInt32 x = 0; x < width; ++x) {
            const UInt8* p = s + x * 3;
            UInt32 v = (UInt32)p[sf.m_byteIndex[B]] << df.m_shift[B] |
                       (UInt32)p[sf.m_byteIndex[G]] << df.m_shift[G] |
                       (UInt32)p[sf.m_byteIndex[R]] << df.m_shift[R];
            if (!useKey || v != key)
                v |= 0xffu << df.m_shift[A];
            dp[x] = v;
        }
        s += src->m_pitch;
        d += dst->m_pitch;
    }
}

// --- 32-bit source -----------------------------------------------------------------------------

void Convert32To16(UInt32 width, UInt32 height, const Src* src, const Dst* dst) // Mac 0x2ad920, Win 0x6a1d80
{
    bool withAlpha = HasAlpha(src->m_pixelFormat) && HasAlpha(dst->m_pixelFormat);
    DitherTo16(width, height, src, dst, 4, withAlpha);
}

void Convert32To24(UInt32 width, UInt32 height, const Src* src, const Dst* dst) // Mac 0x2ae880, Win 0x6a09d0
{
    const BitmapPixelFormat& sf = src->m_pixelFormat;
    const BitmapPixelFormat& df = dst->m_pixelFormat;
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    for (; height != 0; --height) {
        for (UInt32 x = 0; x < width; ++x) {
            d[x * 3 + df.m_byteIndex[R]] = s[x * 4 + sf.m_byteIndex[R]];
            d[x * 3 + df.m_byteIndex[G]] = s[x * 4 + sf.m_byteIndex[G]];
            d[x * 3 + df.m_byteIndex[B]] = s[x * 4 + sf.m_byteIndex[B]];
        }
        s += src->m_pitch;
        d += dst->m_pitch;
    }
}

void Convert32To32(UInt32 width, UInt32 height, const Src* src, const Dst* dst) // Mac 0x2ae9e0, Win 0x6a0a60
{
    const BitmapPixelFormat& sf = src->m_pixelFormat;
    const BitmapPixelFormat& df = dst->m_pixelFormat;
    bool bothAlpha = HasAlpha(sf) && HasAlpha(df);
    bool useKey = src->m_useColorKey && HasAlpha(df);
    UInt32 rgbMask = sf.m_mask[B] | sf.m_mask[G] | sf.m_mask[R];
    UInt32 key = (UInt32)ToByte(src->m_colorKey.b) << sf.m_shift[B] |
                 (UInt32)ToByte(src->m_colorKey.g) << sf.m_shift[G] |
                 (UInt32)ToByte(src->m_colorKey.r) << sf.m_shift[R];
    const UInt8* s = SrcRow(src);
    UInt8* d = DstRow(dst);
    if (sf.m_format == df.m_format) {
        for (; height != 0; --height) {
            if (useKey) {
                const UInt32* sp = reinterpret_cast<const UInt32*>(s);
                UInt32* dp = reinterpret_cast<UInt32*>(d);
                for (UInt32 x = 0; x < width; ++x) {
                    UInt32 masked = sp[x] & rgbMask;
                    dp[x] = masked == key ? masked : sp[x];
                }
            } else {
                memcpy(d, s, width * 4);
            }
            s += src->m_pitch;
            d += dst->m_pitch;
        }
        return;
    }
    for (; height != 0; --height) {
        for (UInt32 x = 0; x < width; ++x) {
            const UInt8* sp = s + x * 4;
            UInt8* dp = d + x * 4;
            dp[df.m_byteIndex[R]] = sp[sf.m_byteIndex[R]];
            dp[df.m_byteIndex[G]] = sp[sf.m_byteIndex[G]];
            dp[df.m_byteIndex[B]] = sp[sf.m_byteIndex[B]];
            if (useKey) {
                if ((*reinterpret_cast<const UInt32*>(sp) & rgbMask) == key)
                    dp[df.m_byteIndex[A]] = 0;
                else
                    dp[df.m_byteIndex[A]] = sp[sf.m_byteIndex[A]];
            } else if (bothAlpha) {
                dp[df.m_byteIndex[A]] = sp[sf.m_byteIndex[A]];
            } else {
                dp[df.m_byteIndex[A]] = 0xff;
            }
        }
        s += src->m_pitch;
        d += dst->m_pitch;
    }
}

} // namespace

// ---------------------------------------------------------------------------------------------
// BitmapPixelFormat
// ---------------------------------------------------------------------------------------------

// Win 0x69f9b0
void BitmapPixelFormat::GetMasks(UInt32 format, UInt32& red, UInt32& green, UInt32& blue, UInt32& alpha)
{
    switch (format) {
    case 0x110:      red = 0xf800;     green = 0x7e0;      blue = 0x1f;       alpha = 0;          return;
    case 0x118:
    case 0x120:      red = 0xff0000;   green = 0xff00;     blue = 0xff;       alpha = 0;          return;
    case 0x210:      red = 0x7c00;     green = 0x3e0;      blue = 0x1f;       alpha = 0;          return;
    case 0x320:      red = 0xff000000; green = 0xff0000;   blue = 0xff00;     alpha = 0;          return;
    case 0x520:      red = 0xff00;     green = 0xff0000;   blue = 0xff000000; alpha = 0;          return;
    case 0x720:      red = 0xff;       green = 0xff00;     blue = 0xff0000;   alpha = 0;          return;
    case 0x40000220: red = 0xff0000;   green = 0xff00;     blue = 0xff;       alpha = 0xff000000; return;
    case 0x40000310: red = 0x7c00;     green = 0x3e0;      blue = 0x1f;       alpha = 0x8000;     return;
    case 0x40000420: red = 0xff000000; green = 0xff0000;   blue = 0xff00;     alpha = 0xff;       return;
    case 0x40000510: red = 0xf00;      green = 0xf0;       blue = 0xf;        alpha = 0xf000;     return;
    case 0x40000620: red = 0xff00;     green = 0xff0000;   blue = 0xff000000; alpha = 0xff;       return;
    case 0x40000820: red = 0xff;       green = 0xff00;     blue = 0xff0000;   alpha = 0xff000000; return;
    default:         red = 0;          green = 0;          blue = 0;          alpha = 0;          return;
    }
}

// Mac 0x2aac30, Win 0x69fc50. Indexed (negative) formats only record palette and format.
void BitmapPixelFormat::Init(UInt32 format, const Color* palette)
{
    m_palette = palette;
    m_format = format;
    for (int c = 0; c < 4; ++c) {
        m_byteIndex[c] = 0;
        m_shift[c] = 0;
        m_loss[c] = 0;
        m_expand[c] = 0;
    }
    if ((Int32)format < 0) {
        m_mask[R] = m_mask[G] = m_mask[B] = m_mask[A] = 0;
        return;
    }
    GetMasks(format, m_mask[R], m_mask[G], m_mask[B], m_mask[A]);
    switch (format) {
    case 0x110:         // 5-6-5
        m_shift[R] = 11; m_shift[G] = 5; m_shift[B] = 0; m_shift[A] = 0x1f;
        m_loss[R] = 3;   m_loss[G] = 2;  m_loss[B] = 3;  m_loss[A] = 0x1f;
        m_expand[R] = kExpand5; m_expand[G] = kExpand6; m_expand[B] = kExpand5; m_expand[A] = kExpand0;
        break;
    case 0x210:         // x-5-5-5
        m_shift[R] = 10; m_shift[G] = 5; m_shift[B] = 0; m_shift[A] = 0x1f;
        m_loss[R] = 3;   m_loss[G] = 3;  m_loss[B] = 3;  m_loss[A] = 0x1f;
        m_expand[R] = m_expand[G] = m_expand[B] = kExpand5; m_expand[A] = kExpand0;
        break;
    case 0x40000310:    // 1-5-5-5
        m_shift[R] = 10; m_shift[G] = 5; m_shift[B] = 0; m_shift[A] = 0xf;
        m_loss[R] = 3;   m_loss[G] = 3;  m_loss[B] = 3;  m_loss[A] = 7;
        m_expand[R] = m_expand[G] = m_expand[B] = kExpand5; m_expand[A] = kExpand1;
        break;
    case 0x410:         // x-4-4-4 (NOTE: GetMasks reports no masks for it: decodes as black)
        m_shift[R] = 8;  m_shift[G] = 4; m_shift[B] = 0; m_shift[A] = 0x1f;
        m_loss[R] = 4;   m_loss[G] = 4;  m_loss[B] = 4;  m_loss[A] = 0x1f;
        m_expand[R] = m_expand[G] = m_expand[B] = kExpand4; m_expand[A] = kExpand0;
        break;
    case 0x40000510:    // 4-4-4-4
        m_shift[R] = 8;  m_shift[G] = 4; m_shift[B] = 0; m_shift[A] = 0xc;
        m_loss[R] = 4;   m_loss[G] = 4;  m_loss[B] = 4;  m_loss[A] = 4;
        m_expand[R] = m_expand[G] = m_expand[B] = m_expand[A] = kExpand4;
        break;
    case 0x118:         // B G R
        m_byteIndex[R] = 2; m_byteIndex[G] = 1; m_byteIndex[B] = 0;
        m_shift[R] = 0x10;  m_shift[G] = 8;     m_shift[B] = 0;     m_shift[A] = 0x18;
        break;
    case 0x120:         // B G R x
    case 0x40000220:    // B G R A
        m_byteIndex[R] = 2; m_byteIndex[G] = 1; m_byteIndex[B] = 0; m_byteIndex[A] = 3;
        m_shift[R] = 0x10;  m_shift[G] = 8;     m_shift[B] = 0;     m_shift[A] = 0x18;
        break;
    case 0x218:         // R G B
        m_byteIndex[R] = 0; m_byteIndex[G] = 1; m_byteIndex[B] = 2;
        m_shift[R] = 0;     m_shift[G] = 8;     m_shift[B] = 0x10;  m_shift[A] = 0x18;
        break;
    case 0x320:         // x B G R
    case 0x40000420:    // A B G R
        m_byteIndex[R] = 3; m_byteIndex[G] = 2; m_byteIndex[B] = 1; m_byteIndex[A] = 0;
        m_shift[R] = 0x18;  m_shift[G] = 0x10;  m_shift[B] = 8;     m_shift[A] = 0;
        break;
    case 0x520:         // x R G B
    case 0x40000620:    // A R G B
        m_byteIndex[R] = 1; m_byteIndex[G] = 2; m_byteIndex[B] = 3; m_byteIndex[A] = 0;
        m_shift[R] = 8;     m_shift[G] = 0x10;  m_shift[B] = 0x18;  m_shift[A] = 0;
        break;
    case 0x720:         // R G B x
    case 0x40000820:    // R G B A
        m_byteIndex[R] = 0; m_byteIndex[G] = 1; m_byteIndex[B] = 2; m_byteIndex[A] = 3;
        m_shift[R] = 0;     m_shift[G] = 8;     m_shift[B] = 0x10;  m_shift[A] = 0x18;
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------------------------------------
// BitmapPixelConverter
// ---------------------------------------------------------------------------------------------

// Mac 0x2ab110 (jump tables on bpp - 1), Win 0x6a2b00.
BitmapPixelConverter::ConvertFunction BitmapPixelConverter::Select(UInt32 sourceFormat, UInt32 destinationFormat)
{
    UInt32 to = destinationFormat & 0xff;
    switch (sourceFormat & 0xff) {
    case 1:
        switch (to) {
        case 1:    return Convert1To1;
        case 2:    return Convert1To2;
        case 4:    return Convert1To4;
        case 8:    return Convert1To8;
        case 0x10: return Convert1To16;
        case 0x18: return Convert1To24;
        case 0x20: return Convert1To32;
        }
        break;
    case 4:
        switch (to) {
        case 4:    return Convert4To4;
        case 8:    return Convert4To8;
        case 0x10: return Convert4To16;
        case 0x18: return Convert4To24;
        case 0x20: return Convert4To32;
        }
        break;
    case 8:
        switch (to) {
        case 8:    return Convert8To8;
        case 0x10: return Convert8To16;
        case 0x18: return Convert8To24;
        case 0x20: return Convert8To32;
        }
        break;
    case 0x10:
        switch (to) {
        case 0x10: return Convert16To16;
        case 0x18: return Convert16To24;
        case 0x20: return Convert16To32;
        }
        break;
    case 0x18:
        switch (to) {
        case 0x10: return Convert24To16;
        case 0x18: return Convert24To24;
        case 0x20: return Convert24To32;
        }
        break;
    case 0x20:
        switch (to) {
        case 0x10: return Convert32To16;
        case 0x18: return Convert32To24;
        case 0x20: return Convert32To32;
        }
        break;
    }
    return 0;
}

// Mac 0x2aef50 (-> 0x2ab110), Win 0x6a2b00
BitmapPixelConverter::BitmapPixelConverter(const BitmapPixelSource* source,
                                           const BitmapPixelDestination* destination)
    : m_function(0), m_source(source), m_destination(destination)
{
    m_function = Select(source->m_pixelFormat.m_format, destination->m_pixelFormat.m_format);
}

} // namespace LEGO
