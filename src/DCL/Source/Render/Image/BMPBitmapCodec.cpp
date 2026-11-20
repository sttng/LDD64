// LEGO Digital Designer — restored source
// TU 299 "BMPBitmapCodec.cpp": Mac 0x2a6d90-0x2aac2f, Win 0x69ae40, 0x69b010-0x69cd0f.
// Names inferred. Platform difference: the palette bytes of Encode (see there).
#include "BMPBitmapCodec.h"
#include "BitmapCodecPixelConverter.h"
#include "Stream.h"
#include <string.h>

namespace LEGO {

namespace {

// An optionally owned byte array (Win: a small {pointer, owns} helper, 0x5034f0 transfer /
// 0x6a0cc0 release; plain locals on the Mac).
class ScopedBuffer {
public:
    ScopedBuffer() : m_data(0), m_owned(false) {}
    ~ScopedBuffer()
    {
        if (m_owned)
            delete[] m_data;
    }
    void Allocate(UInt32 size)
    {
        m_data = new UInt8[size];
        m_owned = m_data != 0;
    }
    UInt8* Get() const { return m_data; }

private:
    ScopedBuffer(const ScopedBuffer&);
    ScopedBuffer& operator=(const ScopedBuffer&);
    UInt8* m_data;
    bool   m_owned;
};

// Win 0x69b290: one 4-bit pixel, high nibble first; `x` counts pixels in the row.
inline void WriteNibble(UInt8*& destination, UInt8 value, UInt32& x)
{
    if ((x & 1) != 0) {
        *destination = (UInt8)((*destination & 0xf0) | (value & 0x0f));
        ++destination;
    } else {
        *destination = (UInt8)((*destination & 0x0f) | (value << 4));
    }
    ++x;
}

// (not in the original; H6) DecodeRLE8 / DecodeRLE4 with every read checked against the
// `length` bytes of `source` and every write against the expanded image: valid data decodes
// exactly as there; when the data ends early or a run, delta or line reaches past the image,
// decoding stops there (a run is cut at the end) and the rest of the image stays 0 (the buffer
// is cleared first).
void DecodeRLE8Checked(const UInt8* source, UInt32 length, UInt8* destination, UInt32 width, UInt32 height)
{
    UInt8* const start = destination;
    UInt8* const end = destination + width * height;
    memset(destination, 0, width * height);
    UInt32 pos = 0;
    for (; height != 0; --height) {
        UInt8 code;
        for (;;) {
            if (length - pos < 2)
                return;
            UInt8 count = source[pos];
            if (count != 0) {
                if ((UInt32)(end - destination) < count) {
                    memset(destination, source[pos + 1], end - destination);   // cut at the end
                    return;
                }
                memset(destination, source[pos + 1], count);
                pos += 2;
                destination += count;
                continue;
            }
            code = source[pos + 1];
            if (code >= 3) {                                     // absolute run, word aligned
                UInt32 available = length - (pos + 2);
                if (available < code || (UInt32)(end - destination) < code) {
                    UInt32 n = (UInt32)(end - destination) < available ? (UInt32)(end - destination) : available;
                    if (n > code)
                        n = code;
                    memcpy(destination, source + pos + 2, n);   // cut at the end
                    return;
                }
                memcpy(destination, source + pos + 2, code);
                destination += code;
                UInt32 advance = (code + 1) & ~1u;
                pos = length - (pos + 2) < advance ? length : pos + 2 + advance;
                continue;
            }
            if (code != 2)
                break;
            if (length - pos < 4)                                // delta
                return;
            UInt8 dx = source[pos + 2];
            UInt8 dy = source[pos + 3];
            pos += 4;
            if (dy == 0) {
                if ((UInt32)(end - destination) < dx)
                    return;
                destination += dx;
            } else {
                if (dy >= height)
                    return;
                UInt32 column = (UInt32)(destination - start) % width;
                UInt32 pad = column != 0 ? width - column : 0;
                UInt64 n = (UInt64)dx + (UInt64)dy * width + pad;
                if ((UInt64)(end - destination) < n)
                    return;
                destination += (UInt32)n;
                height -= dy;
            }
        }
        // end of line (0) or of bitmap (1): the rest of the row (and image) stays 0
        UInt32 column = (UInt32)(destination - start) % width;
        if (column != 0)
            destination += width - column;
        if (code == 1)
            return;
        pos += 2;
    }
}

// One 4-bit pixel as WriteNibble, refused at the end of the expanded image.
inline bool WriteNibbleChecked(UInt8*& destination, const UInt8* end, UInt8 value, UInt32& x)
{
    if (destination >= end)
        return false;
    WriteNibble(destination, value, x);
    return true;
}

void DecodeRLE4Checked(const UInt8* source, UInt32 length, UInt8* destination, UInt32 width, UInt32 height)
{
    const UInt32 rowBytes = (width + 1) >> 1;
    UInt8* const end = destination + rowBytes * height;
    memset(destination, 0, rowBytes * height);
    UInt32 pos = 0;
    for (;;) {
        if (height == 0)
            return;
        UInt32 x = 0;
        UInt8 code;
        for (;;) {
            if (length - pos < 2)
                return;
            UInt8 count = source[pos];
            if (count != 0) {                                    // encoded run of 2 alternating nibbles
                UInt8 value = source[pos + 1];
                pos += 2;
                UInt8 high = (UInt8)(value >> 4);
                UInt8 low = (UInt8)(value & 0x0f);
                do {
                    if (!WriteNibbleChecked(destination, end, high, x))
                        return;
                    if (count == 1)
                        break;
                    if (!WriteNibbleChecked(destination, end, low, x))
                        return;
                    count = (UInt8)(count - 2);
                } while (count != 0);
                continue;
            }
            code = source[pos + 1];
            if (code > 2) {                                      // absolute run
                UInt32 next = pos + 2;
                UInt32 bytes = (code + 1) >> 1;
                if (length - next < bytes)
                    return;
                UInt8 remaining = code;
                while (remaining != 0) {
                    UInt8 value = source[next++];
                    if (!WriteNibbleChecked(destination, end, (UInt8)(value >> 4), x))
                        return;
                    if (remaining == 1)
                        break;
                    if (!WriteNibbleChecked(destination, end, (UInt8)(value & 0x0f), x))
                        return;
                    remaining = (UInt8)(remaining - 2);
                }
                UInt32 pad = (code >> 1) & 1;                    // as the original (see there)
                pos = length - next < pad ? length : next + pad;
                continue;
            }
            if (code != 2)
                break;
            if (length - pos < 4)                                // delta
                return;
            UInt8 dx = source[pos + 2];
            UInt8 dy = source[pos + 3];
            pos += 4;
            if (dy == 0) {
                for (; dx != 0; --dx)
                    if (!WriteNibbleChecked(destination, end, 0, x))
                        return;
            } else {
                if (dy >= height)
                    return;
                UInt32 column = x;
                while (column < width)
                    if (!WriteNibbleChecked(destination, end, 0, column))
                        return;
                if ((column & 1) != 0)
                    ++destination;
                UInt64 n = (UInt64)rowBytes * (dy - 1);
                if (destination > end || (UInt64)(end - destination) < n)
                    return;
                destination += (UInt32)n;
                height -= dy;
                x += dx;
                column = 0;
                while (column < x)
                    if (!WriteNibbleChecked(destination, end, 0, column))
                        return;
            }
        }
        // end of line (0) or of bitmap (1)
        while (x < width)
            if (!WriteNibbleChecked(destination, end, 0, x))
                return;
        if ((x & 1) != 0)
            ++destination;
        if (code == 1)
            return;
        --height;
        pos += 2;
    }
}

} // namespace

// ---------------------------------------------------------------------------------------------

// Mac 0x2a9ae0, Win 0x69ae40 (shared by all four codecs through ICF): drops the cached header.
void BMPBitmapCodec::SetStream(DataStream* stream)
{
    m_header = 0;
    BitmapCodecSurfaceHelper::SetStream(stream);
}

// Win 0x69b010
UInt32 BMPBitmapCodec::GetFormatFromMasks(UInt32 bitCount, UInt32 red, UInt32 green, UInt32 blue, UInt32 alpha)
{
    switch (bitCount) {
    case 1:  return 0x80000101;
    case 2:  return 0x80000102;
    case 4:  return 0x80000104;
    case 8:  return 0x80000108;
    case 0x10:
        if (red == 0xf800) {
            if (green == 0x7e0 && blue == 0x1f && alpha == 0)
                return 0x110;
        } else if (red == 0x7c00) {
            if (green == 0x3e0 && blue == 0x1f) {
                if (alpha == 0)
                    return 0x210;
                if (alpha == 0x8000)
                    return 0x40000310;
            }
        } else if (red == 0xf00 && green == 0xf0 && blue == 0xf) {
            if (alpha == 0)
                return 0x410;
            if (alpha == 0xf000)
                return 0x40000510;
        }
        break;
    case 0x18:
        if (red == 0xff0000) {
            if (green == 0xff00 && blue == 0xff && alpha == 0)
                return 0x118;
        } else if (red == 0xff && green == 0xff00 && blue == 0xff0000 && alpha == 0) {
            return 0x218;
        }
        break;
    case 0x20:
        if (red == 0xff0000) {
            if (green == 0xff00 && blue == 0xff) {
                if (alpha == 0)
                    return 0x120;
                if (alpha == 0xff000000)
                    return 0x40000220;
            }
        } else if (red == 0xff000000) {
            if (green == 0xff0000 && blue == 0xff00) {
                if (alpha == 0)
                    return 0x320;
                if (alpha == 0xff)
                    return 0x40000420;
            }
        } else if (red == 0xff) {
            if (green == 0xff00 && blue == 0xff0000) {
                if (alpha == 0)
                    return 0x720;
                if (alpha == 0xff000000)
                    return 0x40000820;
            }
        } else if (red == 0xff00 && green == 0xff0000 && blue == 0xff000000) {
            if (alpha == 0)
                return 0x520;
            if (alpha == 0xff)
                return 0x40000620;
        }
        break;
    }
    return 0;
}

// Mac 0x2a6d90, Win 0x69c2c0
// Reads at most 0x8a bytes (file header + BITMAPV5HEADER) through a Stream; fields past the end
// of a short file read as 0. Accepts BITMAPCOREHEADER (12) and BITMAPINFOHEADER/V2/V3/V4/V5
// (40, 52, 56, 108, 124).
RefPointer<BitmapCodec::Header> BMPBitmapCodec::ReadHeader()
{
    if (!m_header.IsNull())
        return m_header.Get();
    if (m_stream.IsNull() || m_stream->GetBuffer() == 0 || m_stream->GetBufferSize() == 0)
        return 0;
    UInt32 headerSize = m_stream->GetBufferSize();
    if (headerSize < 0x8a) {
        if (headerSize < 0x1a)
            return 0;
    } else {
        headerSize = 0x8a;
    }

    RefPointer<BMPHeader> header = new BMPHeader;
    Surface surface;
    surface.m_width = 0;
    surface.m_height = 0;
    surface.m_format = 0;
    surface.m_dataSize = 0;

    const UInt8* file = m_stream->GetBuffer();
    Stream stream(const_cast<UInt8*>(file), headerSize, true);
    stream.Read(header->m_fileType);               // byte-swapped: "BM" -> 0x424d
    stream.SetSwap(false);
    stream.Read(header->m_fileSize);
    stream.Read(header->m_reserved1);
    stream.Read(header->m_reserved2);
    stream.Read(header->m_dataOffset);
    UInt32 infoSize;
    stream.Read(infoSize);
    if (header->m_fileType != 0x424d || header->m_fileSize != m_stream->GetBufferSize())
        return 0;
    // NOTE: the info header is read through a Stream of at most 0x8a bytes of the file whatever
    // its declared size (both builds): a file shorter than its info header leaves fields unread;
    // fixed: such a file is refused (H7).
    if ((UInt64)14 + infoSize > header->m_fileSize)
        return 0;
    surface.m_dataSize = header->m_fileSize - infoSize - 14;

    switch (infoSize) {
    case 0xc: {
        // OS/2 BITMAPCOREHEADER: 16-bit sizes, BGR palette triplets.
        header->m_infoSize = 0x28;
        UInt16 value;
        stream.Read(value);
        header->m_width = value;
        stream.Read(value);
        header->m_height = value;
        stream.Read(header->m_planes);
        stream.Read(header->m_bitCount);
        if (header->m_bitCount < 9) {
            UInt32 colors = 1u << header->m_bitCount;
            surface.m_dataSize -= colors * 3;
            UInt32 count = header->m_colorsUsed != 0 ? header->m_colorsUsed : colors;
            if (0x1a + (UInt64)colors * 3 > header->m_fileSize)   // (H7, see below)
                return 0;
            m_palette.Resize(colors, Color());
            const UInt8* entry = file + 0x1a;
            for (UInt32 i = 0; i < count; ++i, entry += 3)
                m_palette.Begin()[i] = Color::FromBytes(entry[2], entry[1], entry[0], 0xff);
        }
        break;
    }
    case 0x28:
    case 0x34:
    case 0x38:
    case 0x6c:
    case 0x7c: {
        header->m_infoSize = infoSize;
        UInt32 value;
        stream.Read(value);
        header->m_width = (Int32)value;
        stream.Read(value);
        header->m_height = (Int32)value;
        stream.Read(header->m_planes);
        stream.Read(header->m_bitCount);
        stream.Read(header->m_compression);
        stream.Read(header->m_imageSize);
        stream.Read(value);
        header->m_xPixelsPerMeter = (Int32)value;
        stream.Read(value);
        header->m_yPixelsPerMeter = (Int32)value;
        stream.Read(header->m_colorsUsed);
        stream.Read(header->m_colorsImportant);
        if (infoSize > 0x28) {
            stream.Read(header->m_redMask);
            stream.Read(header->m_greenMask);
            stream.Read(header->m_blueMask);
        }
        if (infoSize > 0x34)
            stream.Read(header->m_alphaMask);
        if (infoSize > 0x38) {
            stream.Read(header->m_colorSpaceType);
            for (int i = 0; i < 9; ++i)
                stream.Read(header->m_endpoints[i]);
            stream.Read(header->m_gammaRed);
            stream.Read(header->m_gammaGreen);
            stream.Read(header->m_gammaBlue);
        }
        if (infoSize > 0x6c) {
            stream.Read(header->m_intent);
            stream.Read(header->m_profileData);
            stream.Read(header->m_profileSize);
            stream.Read(header->m_reserved);
        }
        if (header->m_bitCount < 9) {
            UInt32 colors = 1u << header->m_bitCount;
            surface.m_dataSize -= colors * 4;
            UInt32 count = header->m_colorsUsed != 0 ? header->m_colorsUsed : colors;
            m_palette.Resize(colors, Color());
            // NOTE: no check of biClrUsed against 1 << bpp or of the palette against the file
            // size (both builds): a larger biClrUsed writes past the palette array; fixed: at
            // most 1 << bpp entries are read and they must lie in the file (H7).
            if (count > colors)
                count = colors;
            if ((UInt64)14 + infoSize + (UInt64)count * 4 > header->m_fileSize)
                return 0;
            const UInt8* entry = file + 14 + infoSize;
            for (UInt32 i = 0; i < count; ++i, entry += 4)
                m_palette.Begin()[i] = Color::FromBytes(entry[2], entry[1], entry[0], 0xff);
        }
        break;
    }
    default:
        return 0;
    }

    if (header->m_planes != 1)
        return 0;
    surface.m_width = (UInt32)header->m_width;
    surface.m_height = (UInt32)(header->m_height < 0 ? -header->m_height : header->m_height);
    if (surface.m_width == 0 || surface.m_height == 0)
        return 0;
    switch (header->m_bitCount) {
    case 1:
        if (header->m_compression != 0)
            return 0;
        surface.m_format = 0x80000101;
        break;
    case 4:
        if (header->m_compression != 0 && header->m_compression != 2)
            return 0;
        surface.m_format = 0x80000104;
        break;
    case 8:
        if (header->m_compression >= 2)
            return 0;
        surface.m_format = 0x80000108;
        break;
    case 0x10:
        if (header->m_compression == 0) {
            surface.m_format = 0x210;
        } else if (header->m_compression == 3) {
            surface.m_format = GetFormatFromMasks(0x10, header->m_redMask, header->m_greenMask,
                                                  header->m_blueMask, header->m_alphaMask);
            if (surface.m_format == 0)
                return 0;
        } else {
            return 0;
        }
        break;
    case 0x18:
        if (header->m_compression != 0)
            return 0;
        surface.m_format = 0x118;
        break;
    case 0x20:
        if (header->m_compression == 0) {
            surface.m_format = 0x40000220;
        } else if (header->m_compression == 3) {
            surface.m_format = GetFormatFromMasks(0x20, header->m_redMask, header->m_greenMask,
                                                  header->m_blueMask, header->m_alphaMask);
            if (surface.m_format == 0)
                return 0;
        } else {
            return 0;
        }
        break;
    default:
        return 0;
    }

    SurfaceArray levels;
    levels.PushBack(surface);
    header->m_surfaces.PushBack(levels);
    m_header = header;
    return header.Get();
}

// Win 0x69b510; inlined in Mac 0x2a7f00.
void BMPBitmapCodec::DecodeRLE8(const UInt8* source, UInt8* destination, UInt32 width, UInt32 height) const
{
    UInt8* start = destination;
    for (; height != 0; --height) {
        UInt8 code;
        const UInt8* next;
        for (;;) {
            UInt8 count = source[0];
            if (count != 0) {
                memset(destination, source[1], count);
                source += 2;
                destination += count;
                continue;
            }
            code = source[1];
            next = source + 2;
            if (code >= 3) {                                     // absolute run, word aligned
                memcpy(destination, next, code);
                destination += code;
                source = next + ((code + 1) & ~1u);
                continue;
            }
            if (code != 2)
                break;
            UInt8 dx = next[0];                                  // delta
            UInt8 dy = source[3];
            source += 4;
            if (dy == 0) {
                memset(destination, 0, dx);
                destination += dx;
            } else {
                UInt32 column = (UInt32)(destination - start) % width;
                UInt32 pad = column != 0 ? width - column : 0;
                UInt32 n = dx + dy * width + pad;
                memset(destination, 0, n);
                destination += n;
                height -= dy;
            }
        }
        // end of line (0) or of bitmap (1): clear the rest of the row
        UInt32 column = (UInt32)(destination - start) % width;
        if (column != 0) {
            memset(destination, 0, width - column);
            destination += width - column;
        }
        if (code == 1) {
            memset(destination, 0, (height - 1) * width);
            destination += (height - 1) * width;
            height = 1;
        }
        source = next;
    }
}

// Mac 0x2a86f0, Win 0x69b2d0.
// NOTE: an absolute run of n pixels is followed by (n >> 1) & 1 padding bytes in both builds
// instead of padding its (n + 1) / 2 data bytes to a word: odd counts are misaligned.
void BMPBitmapCodec::DecodeRLE4(const UInt8* source, UInt8* destination, UInt32 width, UInt32 height) const
{
    for (;;) {
        if (height == 0)
            return;
        UInt32 x = 0;
        UInt8 code;
        const UInt8* next;
        for (;;) {
            UInt8 count = source[0];
            if (count != 0) {                                    // encoded run of 2 alternating nibbles
                UInt8 value = source[1];
                source += 2;
                UInt8 high = (UInt8)(value >> 4);
                UInt8 low = (UInt8)(value & 0x0f);
                do {
                    WriteNibble(destination, high, x);
                    if (count == 1)
                        break;
                    WriteNibble(destination, low, x);
                    count = (UInt8)(count - 2);
                } while (count != 0);
                continue;
            }
            code = source[1];
            next = source + 2;
            if (code > 2) {                                      // absolute run
                UInt8 remaining = code;
                while (remaining != 0) {
                    UInt8 value = *next++;
                    WriteNibble(destination, (UInt8)(value >> 4), x);
                    if (remaining == 1)
                        break;
                    WriteNibble(destination, (UInt8)(value & 0x0f), x);
                    remaining = (UInt8)(remaining - 2);
                }
                source = next + ((code >> 1) & 1);
                continue;
            }
            if (code != 2)
                break;
            UInt8 dx = next[0];                                  // delta
            UInt8 dy = source[3];
            source += 4;
            if (dy == 0) {
                for (; dx != 0; --dx)
                    WriteNibble(destination, 0, x);
            } else {
                UInt32 column = x;
                while (column < width)
                    WriteNibble(destination, 0, column);
                if ((column & 1) != 0)
                    ++destination;
                UInt32 n = ((width + 1) >> 1) * (dy - 1);
                memset(destination, 0, n);
                destination += n;
                height -= dy;
                x += dx;
                column = 0;
                if (x != 0) {
                    do {
                        WriteNibble(destination, 0, column);
                    } while (column < x);
                }
            }
        }
        // end of line (0) or of bitmap (1)
        while (x < width)
            WriteNibble(destination, 0, x);
        if ((x & 1) != 0)
            ++destination;
        if (code == 1) {
            memset(destination, 0, ((width + 1) >> 1) * (height - 1));
            height = 1;
        }
        --height;
        source = next;
    }
}

// Mac 0x2a7f00, Win 0x69b680
// NOTE: rows of bottom-up files are written from row bitmap.m_height - 1 upwards, i.e. a
// bitmap taller than the image receives it at its bottom (both builds).
bool BMPBitmapCodec::Decode(Bitmap& bitmap, const Color* colorKey)
{
    if (bitmap.m_data == 0 || bitmap.m_pitch == 0 || bitmap.m_format == 0 || m_stream.IsNull() ||
        m_stream->GetBuffer() == 0 || m_stream->GetBufferSize() == 0)
        return false;
    if (m_header.IsNull()) {
        RefPointer<Header> header = ReadHeader();
        if (header.IsNull())
            return false;
    }
    const BMPHeader* header = m_header.Get();
    const Surface& surface = header->m_surfaces[0][0];
    UInt32 width = surface.m_width;
    UInt32 height = surface.m_height;
    if (bitmap.m_width < width || bitmap.m_height < height)
        return false;

    // NOTE: bfOffBits is not checked against the file and the rows against the data after it
    // (both builds): an offset past the end, or too little pixel data, reads outside the file;
    // the RLE decoders trust their runs (heap overflow writes, H6); fixed: the offset must lie
    // in the file, uncompressed rows must fit in the data, and the RLE data is decoded with
    // checked reads and writes (H6, H7).
    if (header->m_dataOffset > header->m_fileSize)
        return false;
    const UInt32 dataSize = header->m_fileSize - header->m_dataOffset;
    const bool rle = header->m_compression == 1 || header->m_compression == 2;
    if (!rle && ((UInt64)width * header->m_bitCount + 7) / 8 > dataSize / height)
        return false;
    if (rle && (UInt64)width * height > 0x10000000u)
        return false;
    UInt32 sourcePitch = (header->m_fileSize - header->m_dataOffset) / height;
    const UInt8* sourceData = m_stream->GetBuffer() + header->m_dataOffset;
    UInt8* destinationData = static_cast<UInt8*>(bitmap.m_data);
    Int32 destinationPitch = bitmap.m_pitch;
    const Color* palette = m_palette.Begin();

    BitmapPixelSource source(0, 0, 0, 0);
    ScopedBuffer expanded;
    switch (header->m_bitCount) {
    case 1:
        if (header->m_compression == 0)
            source.Set(surface.m_format, sourceData, (Int32)sourcePitch, palette);
        break;
    case 4:
        if (header->m_compression == 0) {
            source.Set(surface.m_format, sourceData, (Int32)sourcePitch, palette);
        } else if (header->m_compression == 2) {
            UInt32 rowBytes = (width + 1) >> 1;
            expanded.Allocate(rowBytes * height);
            source.Set(surface.m_format, expanded.Get(), (Int32)rowBytes, palette);
            DecodeRLE4Checked(sourceData, dataSize, expanded.Get(), width, height);
        }
        break;
    case 8:
        if (header->m_compression == 0) {
            source.Set(surface.m_format, sourceData, (Int32)sourcePitch, palette);
        } else if (header->m_compression == 1) {
            expanded.Allocate(height * width);
            source.Set(surface.m_format, expanded.Get(), (Int32)width, palette);
            DecodeRLE8Checked(sourceData, dataSize, expanded.Get(), width, height);
        }
        break;
    case 0x10:
    case 0x18:
    case 0x20:
        source.Set(surface.m_format, sourceData, (Int32)sourcePitch, 0);
        break;
    default:
        return false;
    }

    if (header->m_height > 0) {
        destinationData += (Int32)(bitmap.m_height - 1) * destinationPitch;
        destinationPitch = -destinationPitch;
    }
    if (colorKey != 0) {
        source.m_colorKey = *colorKey;
        source.m_useColorKey = true;
    }
    BitmapPixelDestination destination(bitmap.m_format, destinationData, destinationPitch,
                                       (Int32)bitmap.m_format < 0 ? palette : 0);
    BitmapPixelConverter converter(&source, &destination);
    converter.Convert(width, height);

    if ((bitmap.m_format & kBitmapFormatIndexedFlag) != 0 && bitmap.m_palette != 0) {
        UInt32 count = 1u << (bitmap.m_format & 0x1f);
        if (m_palette.GetSize() < count)
            count = m_palette.GetSize();
        for (UInt32 i = 0; i < count; ++i)
            bitmap.m_palette[i] = palette[i];
    }
    return true;
}

// Mac 0x2a8b90, Win 0x69b640: pixels as 32-bit plus file header and a 256-colour palette.
UInt32 BMPBitmapCodec::GetEncodedSize(const Bitmap& bitmap)
{
    return bitmap.m_width * bitmap.m_height * 4 + 0x436;
}

// Mac 0x2a8bb0, Win 0x69bbb0
// Writes an uncompressed BITMAPINFOHEADER file into the stream's buffer (bottom-up rows) and
// shrinks the stream to the file size. Indexed input keeps its palette (written with alpha in
// the reserved byte); 16/24-bit input becomes 24-bit, 32-bit input stays 32-bit (BGRA).
// NOTE: 24/32-bit rows are not padded to 4 bytes; a 2-bit indexed input is converted to 4-bit
// but the file says 2 bits per pixel and its pixel data is not copied (both builds).
bool BMPBitmapCodec::Encode(const Bitmap& bitmap, const EncodeArgs*)
{
    if (bitmap.m_data == 0 || bitmap.m_pitch == 0 || bitmap.m_format == 0 || m_stream.IsNull() ||
        m_stream->GetBuffer() == 0 || m_stream->GetBufferSize() == 0)
        return false;
    UInt32 width = bitmap.m_width;
    UInt32 height = bitmap.m_height;
    UInt16 bitCount = 0;
    UInt32 dataOffset = 0x36;
    UInt32 colors = 0;
    UInt32 paletteReserve = 0;
    UInt32 rowBytes;
    UInt32 fileFormat;
    switch (bitmap.m_format & 0xff) {
    case 1:
        fileFormat = 0x80000101;
        rowBytes = (((width + 7) >> 3) + 3) & ~3u;
        paletteReserve = 8;
        break;
    case 2:
    case 4:
        fileFormat = 0x80000104;
        rowBytes = (((width + 1) >> 1) + 3) & ~3u;
        paletteReserve = 0x40;
        break;
    case 8:
        fileFormat = 0x80000108;
        rowBytes = (width + 3) & ~3u;
        paletteReserve = 0x400;
        break;
    case 0x10:
    case 0x18:
        bitCount = 0x18;
        fileFormat = 0x118;
        rowBytes = width * 3;
        break;
    case 0x20:
        bitCount = 0x20;
        fileFormat = 0x40000220;
        rowBytes = width * 4;
        break;
    default:
        return false;
    }
    if ((Int32)bitmap.m_format < 0) {
        colors = 1u << (bitmap.m_format & 0x1f);
        bitCount = (UInt16)(bitmap.m_format & 0xff);
        dataOffset = colors * 4 + 0x36;
    }
    UInt32 dataSize = rowBytes * height;
    if (m_stream->GetBufferSize() < paletteReserve + dataSize + dataOffset)
        return false;
    m_header = 0;

    UInt8* pixels = new UInt8[dataSize];
    const UInt8* sourceData = static_cast<const UInt8*>(bitmap.m_data);
    Int32 sourcePitch = bitmap.m_pitch;
    if (sourcePitch > 0) {
        sourceData += (Int32)(bitmap.m_height - 1) * sourcePitch;
        sourcePitch = -sourcePitch;
    }
    const Color* palette = (Int32)bitmap.m_format < 0 ? bitmap.m_palette : 0;
    BitmapPixelSource source(bitmap.m_format, sourceData, sourcePitch, palette);
    BitmapPixelDestination destination(fileFormat, pixels, (Int32)rowBytes, palette);
    BitmapPixelConverter converter(&source, &destination);
    converter.Convert(width, height);

    UInt8* file = m_stream->GetBuffer();
    UInt8* out = file + dataOffset;
    switch (bitCount) {
    case 1:
    case 4:
    case 8:
    case 0x18:
    case 0x20:
        memcpy(out, pixels, dataSize);
        out += dataSize;
        break;
    default:
        break;
    }
    UInt32 fileSize = (UInt32)(out - file);
    if (fileSize > m_stream->GetBufferSize()) {
        delete[] pixels;
        return false;
    }

    Stream stream(file, fileSize, true);
    stream.Write((UInt16)0x424d);                  // byte-swapped: "BM"
    stream.SetSwap(false);
    stream.Write(fileSize);
    stream.Write((UInt16)0);
    stream.Write((UInt16)0);
    stream.Write(dataOffset);
    stream.Write((UInt32)0x28);
    stream.Write(width);
    stream.Write(height);
    stream.Write((UInt16)1);
    stream.Write(bitCount);
    stream.Write((UInt32)0);                       // BI_RGB
    stream.Write((UInt32)0);
    stream.Write((UInt32)0xaeb);                   // 2795 pixels per meter = 71 dpi
    stream.Write((UInt32)0xaeb);
    stream.Write(colors);
    stream.Write((UInt32)0);
    if ((Int32)bitmap.m_format < 0 && colors != 0) {
        for (UInt32 i = 0; i < colors; ++i) {
            const Color& c = bitmap.m_palette[i];
            Real32 component[4] = { c.b, c.g, c.r, c.a };
            for (int k = 0; k < 4; ++k) {
                // clamp to [0, 1]; NaN -> 0 (Mac maxss / Win 0x40b4c0, 0x40b4f0)
                Real32 v = component[k];
                if (!(v > 0.0f))
                    v = 0.0f;
                if (!(v < 1.0f))
                    v = 1.0f;
#if defined(_WIN32)
                // x87: clamped value * 255.0 (double), chopped
                stream.Write((UInt8)(Int32)((Real64)v * 255.0));
#else
                // SSE: mulss by 255.0f, cvttss2si
                stream.Write((UInt8)(Int32)(v * 255.0f));
#endif
            }
        }
    }
    m_stream->SetBufferSize(fileSize);
    delete[] pixels;
    return true;
}

} // namespace LEGO
