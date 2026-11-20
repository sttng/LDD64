// LEGO Digital Designer — restored source
// Mac image I/O through ImageIO (see MacImageIO.h). __APPLE__ parts: Mac 0x226aeb-0x226c78,
// 0x1492b0, 0x14e450-0x14e5bf; the !__APPLE__ parts are a HOST-ONLY fallback, NOT restored
// code. Names inferred.
#include "MacImageIO.h"

#if !defined(_WIN32)

#include <string.h>

#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#include <strings.h>
#else
#include "BMPBitmapCodec.h"
#include <png.h>
#include <stdio.h>
extern "C" {
#include <jpeglib.h>
}
#endif

namespace LEGO {

#if defined(__APPLE__)

// Mac 0x226aeb-0x226c78 (inlined in OpenGLTextureFactory::CreateObject, Mac 0x226870)
bool DecodeImageWithImageIO(const void* data, UInt32 size, Bitmap& bitmap)
{
    CGDataProviderRef provider = CGDataProviderCreateWithData(0, data, size, 0);
    CGImageSourceRef source = CGImageSourceCreateWithDataProvider(provider, 0);
    CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, 0);
    size_t width = CGImageGetWidth(image);
    size_t height = CGImageGetHeight(image);
    if (height > kMaxImageIOImageSize || width > kMaxImageIOImageSize) {
        // NOTE: leaks the image, the source and the provider (as the original); fixed: released
        // (low)
        CGImageRelease(image);
        if (source)
            CFRelease(source);
        CGDataProviderRelease(provider);
        return false;
    }
    UInt32 pitch = (UInt32)width * 4;
    UInt32 dataSize = pitch * (UInt32)height;
    UInt8* pixels = new UInt8[dataSize];
    bzero(pixels, dataSize);
    bitmap.m_width = (UInt32)width;
    bitmap.m_height = (UInt32)height;
    bitmap.m_depth = 0;
    bitmap.m_format = kBitmapFormatBGRA32;
    bitmap.m_dataSize = dataSize;
    bitmap.m_pitch = (Int32)pitch;
    bitmap.m_data = pixels;
    bitmap.m_palette = 0;
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceAdobeRGB1998);
    CGContextRef context = CGBitmapContextCreate(pixels, width, height, 8, pitch, colorSpace,
                                                 kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    CGContextDrawImage(context, CGRectMake(0.0f, 0.0f, (CGFloat)width, (CGFloat)height), image);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
    CGImageRelease(image);
    CFRelease(source);
    CGDataProviderRelease(provider);
    return true;
}

// Mac 0x1492b0
RefPointer<DataStreamInterface> EncodePNGWithImageIO(const Bitmap& bitmap)
{
    RefPointer<DataStreamWithDataRef> stream = new DataStreamWithDataRef(CFDataCreateMutable(0, 0));
    CGImageDestinationRef destination = CGImageDestinationCreateWithData(stream->GetDataRef(), kUTTypePNG, 1, 0);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    CGContextRef context = CGBitmapContextCreate(bitmap.m_data, bitmap.m_width, bitmap.m_height, 8, bitmap.m_pitch,
                                                 colorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    CGImageRef image = CGBitmapContextCreateImage(context);
    CGImageDestinationAddImage(destination, image, 0);
    bool finalized = CGImageDestinationFinalize(destination);
    CGImageRelease(image);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
    CFRelease(destination);
    RefPointer<DataStreamInterface> result;
    if (finalized)
        result = stream.Get();
    return result;
}

#else // !__APPLE__

// ---------------------------------------------------------------------------------------------
// HOST-ONLY fallback (NOT restored code): emulates the ImageIO results with libpng / libjpeg.
// ---------------------------------------------------------------------------------------------

namespace {

struct HostImageError {};

inline UInt8 Premultiply(UInt8 c, UInt8 a)
{
    return (UInt8)(((UInt32)c * a + 127) / 255);
}

// RGBA rows (non-premultiplied) -> premultiplied BGRA
void StoreRGBA(const UInt8* rgba, UInt8* bgra, UInt32 count)
{
    for (UInt32 i = 0; i < count; ++i, rgba += 4, bgra += 4) {
        UInt8 a = rgba[3];
        bgra[0] = Premultiply(rgba[2], a);
        bgra[1] = Premultiply(rgba[1], a);
        bgra[2] = Premultiply(rgba[0], a);
        bgra[3] = a;
    }
}

struct PNGReadState {
    const UInt8* data;
    UInt32 size;
    UInt32 position;
};

void HostPNGError(png_structp, png_const_charp) { throw HostImageError(); }
void HostPNGWarning(png_structp, png_const_charp) {}

void HostPNGRead(png_structp png, png_bytep out, png_size_t count)
{
    PNGReadState* state = static_cast<PNGReadState*>(png_get_io_ptr(png));
    if (count > state->size - state->position)
        throw HostImageError();
    memcpy(out, state->data + state->position, count);
    state->position += (UInt32)count;
}

// PNG -> RGBA 8 rows; false when not a PNG / broken
bool DecodePNG(const UInt8* data, UInt32 size, UInt32& width, UInt32& height, UInt8*& rgba)
{
    if (size < 8 || png_sig_cmp(const_cast<png_bytep>(data), 0, 8) != 0)
        return false;
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, HostPNGError, HostPNGWarning);
    png_infop info = png_create_info_struct(png);
    PNGReadState state = { data, size, 0 };
    UInt8* pixels = 0;
    png_bytep* rows = 0;
    try {
        png_set_read_fn(png, &state, HostPNGRead);
        png_read_info(png, info);
        png_set_expand(png);
        png_set_strip_16(png);
        png_set_gray_to_rgb(png);
        png_set_filler(png, 0xff, PNG_FILLER_AFTER);
        png_set_interlace_handling(png);
        png_read_update_info(png, info);
        width = png_get_image_width(png, info);
        height = png_get_image_height(png, info);
        pixels = new UInt8[(size_t)width * height * 4];
        rows = new png_bytep[height];
        for (UInt32 y = 0; y < height; ++y)
            rows[y] = pixels + (size_t)y * width * 4;
        png_read_image(png, rows);
        png_read_end(png, 0);
    } catch (const HostImageError&) {
        delete[] rows;
        delete[] pixels;
        png_destroy_read_struct(&png, &info, 0);
        return false;
    }
    delete[] rows;
    png_destroy_read_struct(&png, &info, 0);
    rgba = pixels;
    return true;
}

struct JPEGError {
    jpeg_error_mgr pub;
};

void HostJPEGErrorExit(j_common_ptr) { throw HostImageError(); }
void HostJPEGOutput(j_common_ptr) {}
void HostJPEGEmit(j_common_ptr, int) {}
void HostJPEGFormat(j_common_ptr, char* buffer) { buffer[0] = 0; }
void HostJPEGReset(j_common_ptr info) { info->err->num_warnings = 0; info->err->msg_code = 0; }

// (jerror.c is not part of the library build — LDD.exe links LEGO's own error manager — so
// jpeg_std_error is not available.)
void InitJPEGError(jpeg_error_mgr& error)
{
    memset(&error, 0, sizeof(error));
    error.error_exit = HostJPEGErrorExit;
    error.emit_message = HostJPEGEmit;
    error.output_message = HostJPEGOutput;
    error.format_message = HostJPEGFormat;
    error.reset_error_mgr = HostJPEGReset;
}

void HostJPEGInitSource(j_decompress_ptr) {}
boolean HostJPEGFill(j_decompress_ptr info)
{
    // end of data: an EOI marker, as libjpeg's own memory sources do
    static const JOCTET eoi[2] = { 0xff, JPEG_EOI };
    info->src->next_input_byte = eoi;
    info->src->bytes_in_buffer = 2;
    return TRUE;
}
void HostJPEGSkip(j_decompress_ptr info, long count)
{
    if (count <= 0)
        return;
    if ((size_t)count > info->src->bytes_in_buffer) {
        HostJPEGFill(info);
        return;
    }
    info->src->next_input_byte += count;
    info->src->bytes_in_buffer -= (size_t)count;
}
void HostJPEGTermSource(j_decompress_ptr) {}

// JPEG -> RGBA 8 rows (alpha 0xff); false when not a JPEG / broken
bool DecodeJPEG(const UInt8* data, UInt32 size, UInt32& width, UInt32& height, UInt8*& rgba)
{
    if (size < 3 || data[0] != 0xff || data[1] != 0xd8)
        return false;
    jpeg_decompress_struct info;
    JPEGError error;
    InitJPEGError(error.pub);
    info.err = &error.pub;
    jpeg_source_mgr source;
    UInt8* pixels = 0;
    UInt8* row = 0;
    try {
        jpeg_create_decompress(&info);
        source.next_input_byte = data;
        source.bytes_in_buffer = size;
        source.init_source = HostJPEGInitSource;
        source.fill_input_buffer = HostJPEGFill;
        source.skip_input_data = HostJPEGSkip;
        source.resync_to_restart = jpeg_resync_to_restart;
        source.term_source = HostJPEGTermSource;
        info.src = &source;
        jpeg_read_header(&info, TRUE);
        info.out_color_space = JCS_RGB;
        jpeg_start_decompress(&info);
        width = info.output_width;
        height = info.output_height;
        pixels = new UInt8[(size_t)width * height * 4];
        row = new UInt8[(size_t)width * 3];
        while (info.output_scanline < info.output_height) {
            UInt8* out = pixels + (size_t)info.output_scanline * width * 4;
            JSAMPROW rows[1] = { row };
            jpeg_read_scanlines(&info, rows, 1);
            for (UInt32 x = 0; x < width; ++x) {
                out[x * 4 + 0] = row[x * 3 + 0];
                out[x * 4 + 1] = row[x * 3 + 1];
                out[x * 4 + 2] = row[x * 3 + 2];
                out[x * 4 + 3] = 0xff;
            }
        }
        jpeg_finish_decompress(&info);
    } catch (const HostImageError&) {
        delete[] row;
        delete[] pixels;
        jpeg_destroy_decompress(&info);
        return false;
    }
    delete[] row;
    jpeg_destroy_decompress(&info);
    rgba = pixels;
    return true;
}

// BMP through the restored codec -> straight BGRA (format 0x40000220)
bool DecodeBMP(const UInt8* data, UInt32 size, UInt32& width, UInt32& height, UInt8*& bgra)
{
    if (size < 2 || data[0] != 'B' || data[1] != 'M')
        return false;
    RefPointer<BMPBitmapCodec> codec = new BMPBitmapCodec;
    codec->SetStream(new DataStream(const_cast<UInt8*>(data), size, false));
    RefPointer<BitmapCodec::Header> header = codec->ReadHeader();
    if (header.IsNull())
        return false;
    const BitmapCodec::Surface& surface = header->m_surfaces[0][0];
    Bitmap out;
    out.m_width = surface.m_width;
    out.m_height = surface.m_height;
    out.m_depth = 0;
    out.m_format = kBitmapFormatBGRA32;
    out.m_pitch = (Int32)(surface.m_width * 4);
    out.m_dataSize = surface.m_width * 4 * surface.m_height;
    out.m_data = new UInt8[out.m_dataSize];
    out.m_palette = 0;
    if (!codec->Decode(out, 0)) {
        delete[] static_cast<UInt8*>(out.m_data);
        return false;
    }
    width = out.m_width;
    height = out.m_height;
    bgra = static_cast<UInt8*>(out.m_data);
    return true;
}

} // namespace

// HOST-ONLY: same contract as the Mac code above.
bool DecodeImageWithImageIO(const void* data, UInt32 size, Bitmap& bitmap)
{
    const UInt8* bytes = static_cast<const UInt8*>(data);
    UInt32 width = 0;
    UInt32 height = 0;
    UInt8* decoded = 0;
    bool straightBGRA = false;
    if (!DecodePNG(bytes, size, width, height, decoded) && !DecodeJPEG(bytes, size, width, height, decoded)) {
        straightBGRA = DecodeBMP(bytes, size, width, height, decoded);
        if (!straightBGRA)
            width = height = 0;   // as ImageIO's null image
    }
    if (height > kMaxImageIOImageSize || width > kMaxImageIOImageSize) {
        delete[] decoded;
        return false;
    }
    UInt32 pitch = width * 4;
    UInt32 dataSize = pitch * height;
    UInt8* pixels = new UInt8[dataSize];
    memset(pixels, 0, dataSize);
    if (decoded != 0) {
        if (straightBGRA) {
            for (UInt32 i = 0; i < dataSize; i += 4) {
                UInt8 a = decoded[i + 3];
                pixels[i + 0] = Premultiply(decoded[i + 0], a);
                pixels[i + 1] = Premultiply(decoded[i + 1], a);
                pixels[i + 2] = Premultiply(decoded[i + 2], a);
                pixels[i + 3] = a;
            }
        } else {
            StoreRGBA(decoded, pixels, width * height);
        }
        delete[] decoded;
    }
    bitmap.m_width = width;
    bitmap.m_height = height;
    bitmap.m_depth = 0;
    bitmap.m_format = kBitmapFormatBGRA32;
    bitmap.m_dataSize = dataSize;
    bitmap.m_pitch = (Int32)pitch;
    bitmap.m_data = pixels;
    bitmap.m_palette = 0;
    return true;
}

void DataStreamWithDataRef::Append(const void* bytes, UInt32 count)
{
    if (m_size + count > m_capacity) {
        UInt32 capacity = m_capacity ? m_capacity * 2 : 0x1000;
        while (capacity < m_size + count)
            capacity *= 2;
        UInt8* grown = new UInt8[capacity];
        if (m_size != 0)
            memcpy(grown, m_bytes, m_size);
        delete[] m_bytes;
        m_bytes = grown;
        m_capacity = capacity;
    }
    memcpy(m_bytes + m_size, bytes, count);
    m_size += count;
}

namespace {

void HostPNGWrite(png_structp png, png_bytep data, png_size_t count)
{
    static_cast<DataStreamWithDataRef*>(png_get_io_ptr(png))->Append(data, (UInt32)count);
}
void HostPNGFlush(png_structp) {}

} // namespace

// HOST-ONLY: premultiplied BGRA -> RGBA 8 PNG (un-premultiplied, as CoreGraphics writes it).
RefPointer<DataStreamInterface> EncodePNGWithImageIO(const Bitmap& bitmap)
{
    RefPointer<DataStreamWithDataRef> stream = new DataStreamWithDataRef;
    RefPointer<DataStreamInterface> result;
    if (bitmap.m_data == 0 || bitmap.m_width == 0 || bitmap.m_height == 0)
        return result;   // CGImageDestinationFinalize fails without an image
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, HostPNGError, HostPNGWarning);
    png_infop info = png_create_info_struct(png);
    UInt8* row = new UInt8[bitmap.m_width * 4];
    try {
        png_set_write_fn(png, stream.Get(), HostPNGWrite, HostPNGFlush);
        png_set_IHDR(png, info, bitmap.m_width, bitmap.m_height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png, info);
        const UInt8* source = static_cast<const UInt8*>(bitmap.m_data);
        for (UInt32 y = 0; y < bitmap.m_height; ++y, source += bitmap.m_pitch) {
            for (UInt32 x = 0; x < bitmap.m_width; ++x) {
                const UInt8* p = source + x * 4;
                UInt8 a = p[3];
                for (int c = 0; c < 3; ++c) {
                    UInt32 v = a ? ((UInt32)p[2 - c] * 255 + a / 2) / a : 0;
                    row[x * 4 + c] = (UInt8)(v > 255 ? 255 : v);
                }
                row[x * 4 + 3] = a;
            }
            png_write_row(png, row);
        }
        png_write_end(png, info);
    } catch (const HostImageError&) {
        delete[] row;
        png_destroy_write_struct(&png, &info);
        return result;
    }
    delete[] row;
    png_destroy_write_struct(&png, &info);
    result = stream.Get();
    return result;
}

#endif // __APPLE__

} // namespace LEGO

#endif // !_WIN32
