// LEGO Digital Designer — restored source
// PNGBitmapCodec (Windows only): Win 0x6991f0-0x6992cf (libpng callbacks), 0x6994e0 (Encode),
// 0x69a2a0-0x69a35f (PNGHeader), 0x69a390 (ReadHeader), 0x69a7e0 (Decode), 0x69ad90-0x69afe0
// (constructor/destructors). Original: libpng 1.5.13 on zlib 1.2.7; LDD64: libpng 1.6.59 on zlib
// 1.4. Names inferred.
//
// Errors: the error and I/O callbacks throw (the original throws the png_structp; wrapped in
// PNGException here); ReadHeader/Decode/Encode catch everything, free libpng and fail.
// Warnings go to libpng's default handler.
#include "PNGBitmapCodec.h"

#if defined(_WIN32)
#include "BitmapCodecPixelConverter.h"
#include <png.h>
#include <string.h>

namespace LEGO {

namespace {

// Position inside the codec's DataStream (read and write callbacks). (name inferred)
struct PNGStreamPosition {
    DataStream* m_stream;
    UInt32      m_position;
};

// What the callbacks throw. The original throws the png_structp itself (throw info 0xa77320);
// a pointer to the incomplete png_struct cannot be thrown in standard C++, so it is wrapped.
struct PNGException {
    explicit PNGException(png_structp png) : m_png(png) {}
    png_structp m_png;
};

// Win 0x6991f0 (error_fn): never returns.
void PNGErrorHandler(png_structp png, png_const_charp)
{
    throw PNGException(png);
}

// Win 0x699210 (read_fn): reading past the stream end is an error.
void PNGReadData(png_structp png, png_bytep data, png_size_t size)
{
    PNGStreamPosition* io = static_cast<PNGStreamPosition*>(png_get_io_ptr(png));
    if (io->m_stream->GetBufferSize() < io->m_position + size)
        throw PNGException(png);
    memcpy(data, io->m_stream->GetBuffer() + io->m_position, size);
    io->m_position += (UInt32)size;
}

// Win 0x699270 (write_fn): writing past the stream end is an error.
void PNGWriteData(png_structp png, png_bytep data, png_size_t size)
{
    PNGStreamPosition* io = static_cast<PNGStreamPosition*>(png_get_io_ptr(png));
    if (io->m_stream->GetBufferSize() < io->m_position + size)
        throw PNGException(png);
    memcpy(io->m_stream->GetBuffer() + io->m_position, data, size);
    io->m_position += (UInt32)size;
}

// Win 0x7076a0 (output_flush_fn; /OPT:ICF folded it with jpeg_mem_term: an empty function).
void PNGFlush(png_structp)
{
}

// Heap array released with delete[] (Win: {pointer, owns} helper, 0x6992d0 / 0x6c9899).
class ScopedBuffer {
public:
    explicit ScopedBuffer(UInt32 size) : m_data(new UInt8[size]) {}
    ~ScopedBuffer() { delete[] m_data; }
    UInt8* Get() const { return m_data; }

private:
    ScopedBuffer(const ScopedBuffer&);
    ScopedBuffer& operator=(const ScopedBuffer&);
    UInt8* m_data;
};

class ScopedRows {
public:
    explicit ScopedRows(UInt32 count) : m_rows(new png_bytep[count]) {}
    ~ScopedRows() { delete[] m_rows; }
    png_bytepp Get() const { return m_rows; }

private:
    ScopedRows(const ScopedRows&);
    ScopedRows& operator=(const ScopedRows&);
    png_bytepp m_rows;
};

} // namespace

// Win 0x69ae40 (ICF with BMPBitmapCodec::SetStream)
void PNGBitmapCodec::SetStream(DataStream* stream)
{
    m_header = 0;
    BitmapCodecSurfaceHelper::SetStream(stream);
}

// Win 0x69eae0 (ICF with JPGBitmapCodec::GetEncodedSize)
UInt32 PNGBitmapCodec::GetEncodedSize(const Bitmap& bitmap)
{
    return bitmap.m_width * bitmap.m_height * 4 + 0x1000;
}

// Win 0x69a390
// Formats: grey 1/2/4/8/16 -> indexed 1/2/4/8 (grey ramp palette made by Decode), RGB 8/16 ->
// 0x218 (16 stripped to 8), palette -> indexed, grey+alpha -> 0xc00001xx (indexed with the alpha
// flag; NOTE: Decode then reads the interleaved grey/alpha bytes as 8-bit indices), RGBA ->
// 0x40000820. Surface::m_dataSize = stream size.
RefPointer<BitmapCodec::Header> PNGBitmapCodec::ReadHeader()
{
    if (!m_header.IsNull())
        return m_header.Get();
    if (m_stream.IsNull() || m_stream->GetBuffer() == 0 || m_stream->GetBufferSize() < 8 ||
        png_sig_cmp(m_stream->GetBuffer(), 0, 8) != 0)
        return 0;

    RefPointer<PNGHeader> header = new PNGHeader;
    Surface surface;
    surface.m_width = 0;
    surface.m_height = 0;
    surface.m_format = 0;
    surface.m_dataSize = m_stream->GetBufferSize();

    png_structp png = 0;
    png_infop info = 0;
    png_infop endInfo = 0;
    try {
        png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, PNGErrorHandler, 0);
        if (png == 0)
            return 0;
        info = png_create_info_struct(png);
        if (info == 0) {
            png_destroy_read_struct(&png, 0, 0);
            return 0;
        }
        endInfo = png_create_info_struct(png);
        if (endInfo == 0) {
            png_destroy_read_struct(&png, &info, 0);
            return 0;
        }
        PNGStreamPosition io;
        io.m_stream = m_stream.Get();
        io.m_position = 0;
        png_set_read_fn(png, &io, PNGReadData);
        png_read_info(png, info);
        png_byte channels = png_get_channels(png, info);
        png_uint_32 width, height;
        int bitDepth, colorType, interlace, compression, filter;
        if (png_get_IHDR(png, info, &width, &height, &bitDepth, &colorType, &interlace,
                         &compression, &filter) == 0) {
            png_destroy_read_struct(&png, &info, &endInfo);
            return 0;
        }
        png_destroy_read_struct(&png, &info, &endInfo);
        surface.m_width = width;
        surface.m_height = height;
        switch (colorType) {
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_PALETTE:
            switch (bitDepth) {
            case 1:  surface.m_format = 0x80000101; break;
            case 2:  surface.m_format = 0x80000102; break;
            case 4:  surface.m_format = 0x80000104; break;
            case 8:  surface.m_format = 0x80000108; break;
            case 16:
                // NOTE: grey 16 -> indexed 8; a 16-bit palette image is invalid PNG anyway
                if (colorType != PNG_COLOR_TYPE_GRAY)
                    return 0;
                surface.m_format = 0x80000108;
                break;
            default: return 0;
            }
            break;
        case PNG_COLOR_TYPE_RGB:
            if (bitDepth != 8 && bitDepth != 16)
                return 0;
            if (channels == 3)
                surface.m_format = Bitmap::SwapRedBlueFormat(0x118);
            else if (channels == 4)
                surface.m_format = 0x720;
            else
                return 0;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            switch (bitDepth) {
            case 1:  surface.m_format = 0xc0000101; break;
            case 2:  surface.m_format = 0xc0000102; break;
            case 4:  surface.m_format = 0xc0000104; break;
            case 8:
            case 16: surface.m_format = 0xc0000108; break;
            default: return 0;
            }
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            if (bitDepth != 8 && bitDepth != 16)
                return 0;
            surface.m_format = 0x40000820;
            break;
        default:
            return 0;
        }
    } catch (...) {
        png_destroy_read_struct(&png, &info, &endInfo);
        return 0;
    }

    SurfaceArray levels;
    levels.PushBack(surface);
    header->m_surfaces.PushBack(levels);
    m_header = header;
    return header.Get();
}

// Win 0x69a7e0
bool PNGBitmapCodec::Decode(Bitmap& bitmap, const Color* colorKey)
{
    if (bitmap.m_data == 0 || bitmap.m_pitch == 0 || bitmap.m_format == 0 || m_stream.IsNull() ||
        m_stream->GetBuffer() == 0 || m_stream->GetBufferSize() == 0)
        return false;
    if (m_header.IsNull()) {
        RefPointer<Header> header = ReadHeader();
        if (header.IsNull())
            return false;
    }
    const Surface& surface = m_header->m_surfaces[0][0];
    UInt32 width = surface.m_width;
    UInt32 height = surface.m_height;
    if (bitmap.m_width < width || bitmap.m_height < height)
        return false;

    png_structp png = 0;
    png_infop info = 0;
    png_infop endInfo = 0;
    try {
        png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, PNGErrorHandler, 0);
        if (png == 0)
            return false;
        info = png_create_info_struct(png);
        if (info == 0) {
            png_destroy_read_struct(&png, &info, 0);
            return false;
        }
        endInfo = png_create_info_struct(png);
        if (endInfo == 0) {
            png_destroy_read_struct(&png, &info, 0);
            return false;
        }
        PNGStreamPosition io;
        io.m_stream = m_stream.Get();
        io.m_position = 0;
        png_set_read_fn(png, &io, PNGReadData);
        png_read_info(png, info);
        png_uint_32 dummyWidth, dummyHeight;
        int bitDepth, colorType, dummy1, dummy2, dummy3;
        if (png_get_IHDR(png, info, &dummyWidth, &dummyHeight, &bitDepth, &colorType, &dummy1,
                         &dummy2, &dummy3) == 0) {
            png_destroy_read_struct(&png, &info, &endInfo);
            return false;
        }
        UInt32 rowBytes = (UInt32)png_get_rowbytes(png, info);
        if (bitDepth == 16) {
            png_set_strip_16(png);
            rowBytes >>= 1;
        }
        ScopedBuffer pixels(rowBytes * height);
        ScopedRows rows(height);
        UInt8* row = pixels.Get();
        for (UInt32 y = 0; y < height; ++y, row += rowBytes)
            rows.Get()[y] = row;
        png_read_image(png, rows.Get());
        png_read_end(png, endInfo);

        BitmapPixelSource source(0, 0, 0, 0);
        UInt32 fileFormat = surface.m_format;
        const Color* palette = 0;
        if ((fileFormat & kBitmapFormatIndexedFlag) != 0) {
            if ((colorType & PNG_COLOR_MASK_COLOR) == 0) {
                // grey: linear ramp 0..1 over 1 << bpp entries (x87: stored as floats)
                UInt32 colors = 1u << (fileFormat & 0x1f);
                m_palette.Resize(colors, Color());
                Real32 step = 1.0f / (Real32)(colors - 1);
                for (UInt32 i = 0; i < colors; ++i) {
                    Real32 v = (Real32)i * step;
                    m_palette.Begin()[i] = Color(v, v, v, 1.0f);
                }
            } else {
                if (png_get_valid(png, info, PNG_INFO_PLTE) == 0) {
                    png_destroy_read_struct(&png, &info, &endInfo);
                    return false;
                }
                png_colorp entries;
                int count;
                png_get_PLTE(png, info, &entries, &count);
                m_palette.Resize(1u << (fileFormat & 0x1f), Color());
                for (int i = 0; i < count; ++i)
                    m_palette.Begin()[i] = Color::FromBytes(entries[i].red, entries[i].green,
                                                            entries[i].blue, 0xff);
            }
            palette = m_palette.Begin();
        }
        source.Set(fileFormat, pixels.Get(), (Int32)rowBytes, palette);
        if (colorKey != 0) {
            source.m_colorKey = *colorKey;
            source.m_useColorKey = true;
        }
        png_destroy_read_struct(&png, &info, &endInfo);

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
    } catch (...) {
        png_destroy_read_struct(&png, &info, &endInfo);
        return false;
    }
}

// Win 0x6994e0
// Indexed input -> palette PNG (bpp from the format, palette without alpha), input with alpha
// -> RGBA 8, else RGB 8; not interlaced, libpng default compression/filters.
bool PNGBitmapCodec::Encode(const Bitmap& bitmap, const EncodeArgs*)
{
    if (bitmap.m_data == 0 || bitmap.m_pitch == 0 || bitmap.m_format == 0 || m_stream.IsNull() ||
        m_stream->GetBuffer() == 0 || m_stream->GetBufferSize() == 0)
        return false;
    UInt32 width = bitmap.m_width;
    UInt32 height = bitmap.m_height;
    png_structp png = 0;
    png_infop info = 0;
    try {
        png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, PNGErrorHandler, 0);
        if (png == 0)
            return false;
        info = png_create_info_struct(png);
        if (info == 0) {
            png_destroy_write_struct(&png, 0);
            return false;
        }
        PNGStreamPosition io;
        io.m_stream = m_stream.Get();
        io.m_position = 0;
        png_set_write_fn(png, &io, PNGWriteData, PNGFlush);

        int colorType;
        int bitDepth;
        UInt32 fileFormat;
        if ((Int32)bitmap.m_format < 0) {
            colorType = PNG_COLOR_TYPE_PALETTE;
            bitDepth = (int)(bitmap.m_format & 0xff);
            fileFormat = (bitmap.m_format & 0xff) | 0x80000100;
        } else if ((bitmap.m_format & kBitmapFormatAlphaFlag) == 0) {
            colorType = PNG_COLOR_TYPE_RGB;
            fileFormat = Bitmap::SwapRedBlueFormat(0x118);
            bitDepth = 8;
        } else {
            colorType = PNG_COLOR_TYPE_RGB_ALPHA;
            fileFormat = 0x40000820;
            bitDepth = 8;
        }
        png_set_IHDR(png, info, width, height, bitDepth, colorType, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        UInt32 rowBytes = (UInt32)png_get_rowbytes(png, info);
        if (rowBytes == 0) {
            png_destroy_write_struct(&png, &info);
            return false;
        }
        if ((bitmap.m_format & kBitmapFormatIndexedFlag) != 0) {
            UInt32 colors = 1u << (bitmap.m_format & 0x1f);
            ScopedBuffer entries(colors * 3);
            for (UInt32 i = 0; i < colors; ++i) {
                const Color& c = bitmap.m_palette[i];
                // x87 product with the double 255.0, chopped (no clamping)
                entries.Get()[i * 3 + 0] = (UInt8)(Int32)((Real64)c.r * 255.0);
                entries.Get()[i * 3 + 1] = (UInt8)(Int32)((Real64)c.g * 255.0);
                entries.Get()[i * 3 + 2] = (UInt8)(Int32)((Real64)c.b * 255.0);
            }
            png_set_PLTE(png, info, reinterpret_cast<png_colorp>(entries.Get()), (int)colors);
        }
        png_write_info(png, info);

        ScopedBuffer pixels(rowBytes * height);
        const Color* palette = (Int32)bitmap.m_format < 0 ? bitmap.m_palette : 0;
        BitmapPixelSource source(bitmap.m_format, bitmap.m_data, bitmap.m_pitch, palette);
        BitmapPixelDestination destination(fileFormat, pixels.Get(), (Int32)rowBytes,
                                           (bitmap.m_format & kBitmapFormatIndexedFlag) != 0 ? bitmap.m_palette : 0);
        BitmapPixelConverter converter(&source, &destination);
        converter.Convert(width, height);

        ScopedRows rows(height);
        UInt8* row = pixels.Get();
        for (UInt32 y = 0; y < height; ++y, row += rowBytes)
            rows.Get()[y] = row;
        png_write_image(png, rows.Get());
        png_write_end(png, 0);
        png_destroy_write_struct(&png, &info);
        m_stream->SetBufferSize(io.m_position);
        return true;
    } catch (...) {
        png_destroy_write_struct(&png, &info);
        return false;
    }
}

} // namespace LEGO

#endif // _WIN32
