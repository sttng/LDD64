// LEGO Digital Designer — restored source
// PCXBitmapCodec (Windows only): Win 0x69cd10-0x69e33f. Names inferred.
//
// Supported: 1 bit x 1 plane (black/white), 8 bits x 1 plane (256-colour palette at the end of
// the file, marker 0x0c), 8 bits x 3 planes (RGB). Run-length runs continue across the colour
// planes of a row (both directions), unlike the PCX specification.
#include "PCXBitmapCodec.h"

#if defined(_WIN32)
#include "BitmapCodecPixelConverter.h"
#include "Stream.h"
#include <string.h>

namespace LEGO {

namespace {

// Win 0x69cd10: RLE without regard to rows; decodes until rowBytes * height bytes are written.
// NOTE: a run crossing the end writes past the buffer (no clipping).
void DecodeRLE(const UInt8* source, UInt8* destination, UInt32 rowBytes, UInt32 height)
{
    UInt8* end = destination + rowBytes * height;
    while (destination < end) {
        UInt8 b = *source;
        if (b < 0xc0) {
            *destination++ = b;
            ++source;
        } else {
            memset(destination, source[1], b & 0x3f);
            destination += b & 0x3f;
            source += 2;
        }
    }
}

// Win 0x69cd60: RLE of planar rows (all of plane 0, then 1, then 2) into interleaved pixels at
// the byte positions `offsets` (stride 3).
void DecodePlanarRLE(const UInt8* source, UInt8* destination, UInt32 rowBytes, UInt32 height,
                     const UInt32* offsets)
{
    UInt32 position = offsets[0];
    UInt8* end = destination + rowBytes * height;
    UInt32 plane = 0;
    while (destination < end) {
        UInt8 value = *source++;
        UInt32 count = 1;
        if (value > 0xbf) {
            count = value & 0x3f;
            value = *source++;
            if (count == 0)
                continue;
        }
        do {
            destination[position] = value;
            position += 3;
            --count;
            if (rowBytes <= position) {
                ++plane;
                if (plane < 3) {
                    position = offsets[plane];
                } else if (plane == 3) {
                    position = offsets[0];
                    plane = 0;
                    destination += rowBytes;
                }
            }
        } while (count != 0);
    }
}

// (not in the original) DecodeRLE / DecodePlanarRLE with the reads checked against the
// `length` bytes of `source` and the writes against the image: valid data decodes exactly as
// there; a run crossing the end is cut, data that ends early leaves the rest 0.
void DecodeRLEChecked(const UInt8* source, UInt32 length, UInt8* destination, UInt32 rowBytes, UInt32 height)
{
    UInt8* end = destination + rowBytes * height;
    memset(destination, 0, rowBytes * height);
    UInt32 pos = 0;
    while (destination < end && pos < length) {
        UInt8 b = source[pos];
        if (b < 0xc0) {
            *destination++ = b;
            ++pos;
        } else {
            if (length - pos < 2)
                return;
            UInt32 count = b & 0x3f;
            if (count > (UInt32)(end - destination))
                count = (UInt32)(end - destination);
            memset(destination, source[pos + 1], count);
            destination += count;
            pos += 2;
        }
    }
}

void DecodePlanarRLEChecked(const UInt8* source, UInt32 length, UInt8* destination, UInt32 rowBytes,
                            UInt32 height, const UInt32* offsets)
{
    UInt32 position = offsets[0];
    UInt8* end = destination + rowBytes * height;
    memset(destination, 0, rowBytes * height);
    UInt32 plane = 0;
    UInt32 pos = 0;
    while (destination < end) {
        if (pos >= length)
            return;
        UInt8 value = source[pos++];
        UInt32 count = 1;
        if (value > 0xbf) {
            if (pos >= length)
                return;
            count = value & 0x3f;
            value = source[pos++];
            if (count == 0)
                continue;
        }
        do {
            destination[position] = value;
            position += 3;
            --count;
            if (rowBytes <= position) {
                ++plane;
                if (plane < 3) {
                    position = offsets[plane];
                } else if (plane == 3) {
                    position = offsets[0];
                    plane = 0;
                    destination += rowBytes;
                    if (destination >= end)
                        return;
                }
            }
        } while (count != 0);
    }
}

inline void FlushRun(UInt8*& out, UInt32 run, UInt8 value)
{
    if (run != 1 || value > 0xbf)
        *out++ = (UInt8)(run | 0xc0);
    *out++ = value;
}

// Win 0x69cdf0: RLE per row (runs end with the row, at most 63; a run reaching 63 is written as
// 0xff value at once).
void EncodeRLE(const UInt8* source, UInt8*& out, UInt32 rowBytes, UInt32 height)
{
    const UInt8* end = source + rowBytes * height;
    UInt32 run = 0;
    UInt32 remaining = rowBytes;
    UInt8 value = (UInt8)rowBytes;
    while (source < end) {
        UInt8 b = *source++;
        if (run == 0) {
            run = 1;
            value = b;
        } else if (b != value) {
            FlushRun(out, run, value);
            run = 1;
            value = b;
        } else {
            ++run;
            if (run == 0x3f) {
                *out++ = 0xff;
                *out++ = b;
                run = 0;
            }
        }
        if (--remaining == 0) {
            remaining = rowBytes;
            if (run != 0)
                FlushRun(out, run, value);
            run = 0;
        }
    }
    if (run != 0)
        FlushRun(out, run, value);
}

// Win 0x69cec0: interleaved pixels (stride 3, `offsets` per plane) -> planar RLE rows; runs end
// with the row only.
void EncodePlanarRLE(const UInt8* source, UInt8*& out, UInt32 width, UInt32 height, const UInt32* offsets)
{
    UInt32 rowBytes = width * 3;
    const UInt8* end = source + rowBytes * height;
    UInt32 position = offsets[0];
    UInt32 plane = 0;
    UInt32 run = 0;
    UInt8 value = 0;   // (the original starts from a garbage byte; it is never used)
    while (source < end) {
        UInt8 b = source[position];
        if (run == 0) {
            run = 1;
            value = b;
        } else if (b != value) {
            FlushRun(out, run, value);
            run = 1;
            value = b;
        } else {
            ++run;
            if (run == 0x3f) {
                *out++ = 0xff;
                *out++ = b;
                run = 0;
            }
        }
        position += 3;
        if (rowBytes <= position) {
            ++plane;
            if (plane < 3) {
                position = offsets[plane];
            } else if (plane == 3) {
                source += rowBytes;
                position = offsets[0];
                plane = 0;
                if (run != 0)
                    FlushRun(out, run, value);
                run = 0;
            }
        }
    }
    if (run != 0)
        FlushRun(out, run, value);
}

class ScopedBuffer {
public:
    ScopedBuffer() : m_data(0) {}
    ~ScopedBuffer() { delete[] m_data; }
    void Allocate(UInt32 size) { m_data = new UInt8[size]; }
    UInt8* Get() const { return m_data; }

private:
    ScopedBuffer(const ScopedBuffer&);
    ScopedBuffer& operator=(const ScopedBuffer&);
    UInt8* m_data;
};

} // namespace

// Win 0x69ae40 (ICF with BMPBitmapCodec::SetStream)
void PCXBitmapCodec::SetStream(DataStream* stream)
{
    m_header = 0;
    BitmapCodecSurfaceHelper::SetStream(stream);
}

// Win 0x69cfd0
UInt32 PCXBitmapCodec::GetEncodedSize(const Bitmap& bitmap)
{
    return bitmap.m_width * bitmap.m_height * 8 + 0x380;
}

// Win 0x69dc70
// NOTE: the header is read through a Stream of 0x80 bytes without checking the stream size
// (its fields take 74 bytes: a shorter file is read past its end), and the 256-colour palette
// is looked for 0x301 bytes before the end of the stream without a size check either (in a
// file shorter than that, before its start). Fixed: files shorter than either are refused.
RefPointer<BitmapCodec::Header> PCXBitmapCodec::ReadHeader()
{
    if (!m_header.IsNull())
        return m_header.Get();
    if (m_stream.IsNull() || m_stream->GetBuffer() == 0 || m_stream->GetBufferSize() == 0)
        return 0;
    RefPointer<PCXHeader> header = new PCXHeader;
    Surface surface;
    surface.m_width = 0;
    surface.m_height = 0;
    surface.m_format = 0;
    surface.m_dataSize = 0;

    const UInt8* file = m_stream->GetBuffer();
    UInt32 fileSize = m_stream->GetBufferSize();
    if (fileSize < 0x80)                                       // (see above)
        return 0;
    Stream stream(const_cast<UInt8*>(file), 0x80, false);
    stream.Read(header->m_manufacturer);
    stream.Read(header->m_version);
    stream.Read(header->m_encoding);
    stream.Read(header->m_bitsPerPixel);
    if (header->m_manufacturer != 10 || header->m_version != 5 || header->m_encoding > 1 ||
        (header->m_bitsPerPixel != 1 && header->m_bitsPerPixel != 8))
        return 0;
    UInt16 value;
    stream.Read(value);
    header->m_xMin = (Int16)value;
    stream.Read(value);
    header->m_yMin = (Int16)value;
    stream.Read(value);
    header->m_xMax = (Int16)value;
    stream.Read(value);
    header->m_yMax = (Int16)value;
    stream.Read(header->m_horizontalDpi);
    stream.Read(header->m_verticalDpi);
    for (int i = 0; i < 48; ++i)
        stream.Read(header->m_palette[i]);
    stream.Read(header->m_reserved);
    stream.Read(header->m_planes);
    stream.Read(header->m_bytesPerLine);
    stream.Read(header->m_paletteInfo);
    stream.Read(header->m_horizontalScreenSize);
    stream.Read(header->m_verticalScreenSize);

    if (header->m_xMax < header->m_xMin || header->m_yMax < header->m_yMin)
        return 0;   // a negative extent wraps to a size near 4G
    surface.m_width = (UInt32)((Int32)header->m_xMax - (Int32)header->m_xMin + 1);
    surface.m_height = (UInt32)((Int32)header->m_yMax - (Int32)header->m_yMin + 1);
    if (surface.m_width == 0 || surface.m_height == 0)
        return 0;
    surface.m_dataSize = fileSize - 0x80;
    if (header->m_bitsPerPixel == 1) {
        surface.m_format = 0x80000101;
        m_palette.Resize(2, Color());
        m_palette.Begin()[0] = Color(0.0f, 0.0f, 0.0f, 1.0f);
        m_palette.Begin()[1] = Color(1.0f, 1.0f, 1.0f, 1.0f);
    } else {
        // 8 bits per pixel
        if (header->m_planes == 1) {
            if (fileSize < 0x80 + 0x301)
                return 0;
            const UInt8* marker = file + fileSize - 0x301;
            surface.m_dataSize = fileSize - 0x380;
            surface.m_format = 0x80000108;
            if (*marker != 0x0c)
                return 0;
            m_palette.Resize(0x100, Color());
            const UInt8* entry = marker + 1;
            for (UInt32 i = 0; i < 0x100; ++i, entry += 3)
                m_palette.Begin()[i] = Color::FromBytes(entry[0], entry[1], entry[2], 0xff);
        } else if (header->m_planes == 3) {
            surface.m_format = 0x218;
        } else {
            return 0;
        }
    }
    SurfaceArray levels;
    levels.PushBack(surface);
    header->m_surfaces.PushBack(levels);
    m_header = header;
    return header.Get();
}

// Win 0x69d020
// NOTE: reads the cached header's surface before any check (crashes on a codec whose header
// was never read; the factory always reads it). Source rows are (width + 7) / 8, (width + 1) & ~1
// and ((width + 1) & ~1) * 3 bytes: the bytes-per-line field of the file is not used.
bool PCXBitmapCodec::Decode(Bitmap& bitmap, const Color* colorKey)
{
    if (bitmap.m_data == 0 || bitmap.m_pitch == 0 || bitmap.m_format == 0 || m_stream.IsNull() ||
        m_stream->GetBuffer() == 0 || m_stream->GetBufferSize() == 0)
        return false;
    if (m_header.IsNull()) {
        RefPointer<Header> header = ReadHeader();
        if (header.IsNull())
            return false;
    }
    const Surface* surface = &m_header->m_surfaces[0][0];
    const PCXHeader* header = m_header.Get();
    UInt32 width = surface->m_width;
    UInt32 height = surface->m_height;
    if (bitmap.m_width < width || bitmap.m_height < height)
        return false;

    const UInt8* data = m_stream->GetBuffer() + 0x80;
    // NOTE: the pixel data is not checked against the file (both builds): raw rows are read and
    // RLE runs decoded past its end, and a run crossing the end of the image writes past the
    // buffer; fixed: raw rows must fit in the file, RLE is decoded with checked reads and
    // writes, and an image of more than 256 MB is refused.
    const UInt32 dataSize = m_stream->GetBufferSize() - 0x80 -
                            ((Int8)header->m_planes == 1 && (Int8)header->m_bitsPerPixel == 8 ? 0x301 : 0);
    {
        Int32 bitsChecked = (Int32)(Int8)header->m_planes * (Int32)(Int8)header->m_bitsPerPixel;
        UInt64 rowBytesChecked = bitsChecked == 1 ? (width + 7) >> 3
                               : bitsChecked == 8 ? (width + 1) & ~1u
                               : ((UInt64)(width + 1) & ~1ull) * 3;
        UInt64 imageBytes = rowBytesChecked * height;
        if (imageBytes > 0x10000000u || (header->m_encoding == 0 && imageBytes > dataSize))
            return false;
    }
    BitmapPixelSource source(0, 0, 0, 0);
    ScopedBuffer expanded;
    Int32 bits = (Int32)(Int8)header->m_planes * (Int32)(Int8)header->m_bitsPerPixel;
    if (bits == 1 || bits == 8) {
        UInt32 rowBytes = bits == 1 ? (width + 7) >> 3 : (width + 1) & ~1u;
        if (header->m_encoding == 0) {
            source.Set(surface->m_format, data, (Int32)rowBytes, m_palette.Begin());
        } else if (header->m_encoding == 1) {
            expanded.Allocate(rowBytes * height);
            source.Set(surface->m_format, expanded.Get(), (Int32)rowBytes, m_palette.Begin());
            DecodeRLEChecked(data, dataSize, expanded.Get(), rowBytes, height);
        }
    } else if (bits == 0x18) {
        UInt32 rowBytes = ((width + 1) & ~1u) * 3;
        if (header->m_encoding == 0) {
            // NOTE: raw planar rows are converted as if they were interleaved
            source.Set(surface->m_format, data, (Int32)rowBytes, 0);
        } else if (header->m_encoding == 1) {
            expanded.Allocate(rowBytes * height);
            source.Set(surface->m_format, expanded.Get(), (Int32)rowBytes, 0);
            UInt32 offsets[3];
            offsets[0] = source.m_pixelFormat.m_byteIndex[0];
            offsets[1] = source.m_pixelFormat.m_byteIndex[1];
            offsets[2] = source.m_pixelFormat.m_byteIndex[2];
            DecodePlanarRLEChecked(data, dataSize, expanded.Get(), rowBytes, height, offsets);
        }
    } else {
        return false;
    }

    if (colorKey != 0) {
        source.m_colorKey = *colorKey;
        source.m_useColorKey = true;
    }
    BitmapPixelDestination destination(bitmap.m_format, bitmap.m_data, bitmap.m_pitch,
                                       (Int32)bitmap.m_format < 0 ? m_palette.Begin() : 0);
    BitmapPixelConverter converter(&source, &destination);
    converter.Convert(width, height);
    if ((bitmap.m_format & kBitmapFormatIndexedFlag) != 0 && bitmap.m_palette != 0) {
        UInt32 count = 1u << (bitmap.m_format & 0x1f);
        if (m_palette.GetSize() < count)
            count = m_palette.GetSize();
        for (UInt32 i = 0; i < count; ++i)
            bitmap.m_palette[i] = m_palette.Begin()[i];
    }
    return true;
}

// Win 0x69d5b0
// Only non-indexed input: 16/24/32-bit -> 8 bits x 3 planes RLE. (The 1-bit and 2/4/8-bit
// branches exist but cannot be reached: their formats are indexed.)
// NOTE: rows are width * 3 bytes (not padded to even) while the header says width bytes per
// line; for the indexed branches the file size includes a 256-colour palette that is never
// written; 24-bit files with odd widths do not decode back (Decode pads rows to even).
bool PCXBitmapCodec::Encode(const Bitmap& bitmap, const EncodeArgs*)
{
    if (bitmap.m_data == 0 || bitmap.m_pitch == 0 || bitmap.m_format == 0 || m_stream.IsNull() ||
        m_stream->GetBuffer() == 0 || m_stream->GetBufferSize() == 0 || (Int32)bitmap.m_format < 0)
        return false;
    UInt32 height = bitmap.m_height;
    UInt32 width = bitmap.m_width;
    Int16 shortWidth = (Int16)width;
    UInt8 bitsPerPixel;
    UInt8 planes;
    Int16 bytesPerLine;
    UInt32 fileFormat;
    UInt32 rowBytes;
    UInt32 paletteReserve = 0;
    switch (bitmap.m_format & 0xff) {
    case 1:
        bitsPerPixel = 1;
        planes = 1;
        bytesPerLine = (Int16)((width + 7) >> 3);
        fileFormat = 0x80000101;
        rowBytes = (UInt32)(Int32)bytesPerLine;
        paletteReserve = 6;
        break;
    case 2:
    case 4:
    case 8:
        planes = 1;
        fileFormat = 0x80000108;
        rowBytes = (UInt32)(Int32)shortWidth;
        paletteReserve = 0x300;
        bitsPerPixel = 8;
        bytesPerLine = shortWidth;
        break;
    case 0x10:
    case 0x18:
    case 0x20:
        planes = 3;
        fileFormat = 0x218;
        rowBytes = (UInt32)(shortWidth * 3);
        bitsPerPixel = 8;
        bytesPerLine = shortWidth;
        break;
    default:
        return false;
    }
    UInt32 dataSize = rowBytes * height;
    if (m_stream->GetBufferSize() < dataSize + 0x80 + paletteReserve)
        return false;
    m_header = 0;

    UInt8* pixels = new UInt8[dataSize];
    BitmapPixelSource source(bitmap.m_format, bitmap.m_data, bitmap.m_pitch, 0);
    BitmapPixelDestination destination(fileFormat, pixels, (Int32)rowBytes, 0);
    BitmapPixelConverter converter(&source, &destination);
    converter.Convert(width, height);

    UInt8* file = m_stream->GetBuffer();
    UInt8* out = file + 0x80;
    Int32 bits = (Int32)bitsPerPixel * (Int32)planes;
    if (bits == 1 || bits == 8) {
        EncodeRLE(pixels, out, rowBytes, height);
    } else if (bits == 0x18) {
        UInt32 offsets[3] = { 0, 1, 2 };
        EncodePlanarRLE(pixels, out, width, height, offsets);
    }
    UInt32 written = (UInt32)(out - file);
    if (m_stream->GetBufferSize() < written + paletteReserve) {
        delete[] pixels;
        return false;
    }

    Stream stream(file, 0x80, false);
    stream.Write((UInt8)10);                       // manufacturer
    stream.Write((UInt8)5);                        // version
    stream.Write((UInt8)1);                        // RLE
    stream.Write(bitsPerPixel);
    stream.Write((UInt16)0);                       // xMin
    stream.Write((UInt16)0);                       // yMin
    stream.Write((UInt16)(width - 1));             // xMax
    stream.Write((UInt16)(height - 1));            // yMax
    stream.Write((UInt16)0x47);                    // 71 dpi
    stream.Write((UInt16)0x47);
    for (UInt32 i = 0; i < 16; ++i) {              // EGA palette: grey ramp 15..0
        UInt8 grey = (UInt8)(15 - i);
        stream.Write(grey);
        stream.Write(grey);
        stream.Write(grey);
    }
    stream.Write((UInt8)0);                        // reserved
    stream.Write(planes);
    stream.Write((UInt16)bytesPerLine);
    stream.Write((UInt16)1);                       // palette info: colour
    stream.Write((UInt16)0);
    stream.Write((UInt16)0);
    UInt8 filler[0x36];
    memset(filler, 0, sizeof(filler));
    stream.Write(filler, sizeof(filler));

    UInt32 size = written;
    if ((Int32)fileFormat < 0)
        size += (1u << (fileFormat & 0x1f)) * 3;
    m_stream->SetBufferSize(size);
    delete[] pixels;
    return true;
}

} // namespace LEGO

#endif // _WIN32
