// LEGO Digital Designer — restored source
// JPGBitmapCodec and its libjpeg wrappers (Windows only): Win 0x69e340-0x69f9af.
// libjpeg (original 6b, LDD64 10; boolean is unsigned char); these classes replace
// jerror.c, jdatasrc.c and jdatadst.c. Class names from RTTI, members inferred.
// Each libjpeg callback is a static trampoline to a virtual of the manager (via
// cinfo->client_data = JCommon). Fatal errors throw the JErrorManager by value; the codec
// catches everything and fails.
#include "JPGBitmapCodec.h"

#if defined(_WIN32)
#include "BitmapCodecPixelConverter.h"
#include <stdio.h>
#include <string.h>
// <windows.h> (through Object.h) already defines INT32 (basetsd.h); XMD_H keeps jmorecfg.h from
// redefining it as long (same size on Win32: the library's ABI is unchanged).
#define XMD_H
extern "C" {
#include <jpeglib.h>
}

namespace LEGO {

namespace {

class JCommon;

// Win vtable 0x9de47c {sdd 0x69e420, ErrorExit 0x69ebf0, EmitMessage 0x7f3490 (no-op),
// OutputMessage 0x713eb0, FormatMessage 0x7f3490, ResetErrorManager 0x713eb0}; ctor 0x69eba0.
// sizeof 0x88.
class JErrorManager {
public:
    JErrorManager();                                                 // Win 0x69eba0
    virtual ~JErrorManager() {}                                      // Win 0x69e370 / 0x69e420
    virtual void ErrorExit(j_common_ptr)                             // Win 0x69ebf0
    {
        throw *this;
    }
    virtual void EmitMessage(j_common_ptr, int) {}
    virtual void OutputMessage(j_common_ptr) {}
    virtual void FormatMessage(j_common_ptr, char*) {}
    virtual void ResetErrorManager(j_common_ptr) {}

    jpeg_error_mgr m_manager;                                        // +0x04
};

// Win vtable 0x9de4bc {sdd 0x69e860, InitSource 0x713eb0, FillInputBuffer 0x7c2400 (false:
// suspend), SkipInputData 0x7f3490, ResyncToRestart 0x69e790, TermSource 0x713eb0}. sizeof 0x20.
class JSourceManager {
public:
    JSourceManager();                                                // Win 0x69ec30 (first part)
    virtual ~JSourceManager() {}                                     // Win 0x69e780 / 0x69e860
    virtual void InitSource(j_decompress_ptr) {}
    virtual boolean FillInputBuffer(j_decompress_ptr) { return FALSE; }
    virtual void SkipInputData(j_decompress_ptr, long) {}
    virtual boolean ResyncToRestart(j_decompress_ptr cinfo, int desired)  // Win 0x69e790
    {
        return jpeg_resync_to_restart(cinfo, desired) != 0;
    }
    virtual void TermSource(j_decompress_ptr) {}

    jpeg_source_mgr m_manager;                                       // +0x04
};

// The whole file in memory. Win vtable 0x9de4d8 {sdd 0x69e9a0, InitSource 0x69e980, ..,
// SkipInputData 0x69e990, ..}, ctor 0x69ec30. sizeof 0x28.
class MemSourceManager : public JSourceManager {
public:
    MemSourceManager(const UInt8* data, UInt32 size) : m_data(data), m_size(size) {}
    virtual ~MemSourceManager() {}                                   // Win 0x69e930 / 0x69e9a0
    virtual void InitSource(j_decompress_ptr)                        // Win 0x69e980
    {
        m_manager.next_input_byte = m_data;
        m_manager.bytes_in_buffer = m_size;
    }
    // NOTE: also *adds* the count to bytes_in_buffer (should subtract): after a skipped marker
    // (APPn other than JFIF/Adobe, COM) libjpeg believes 2 * n more bytes remain, so a truncated
    // file is read past its end instead of suspending.
    virtual void SkipInputData(j_decompress_ptr, long count)        // Win 0x69e990
    {
        m_manager.next_input_byte += count;
        m_manager.bytes_in_buffer += count;
    }

    const UInt8* m_data;                                             // +0x20
    UInt32       m_size;                                             // +0x24
};

// Win vtable 0x9de4a8 {sdd 0x69e6b0, InitDestination 0x713eb0, EmptyOutputBuffer 0x7c2400
// (false: suspend), TermDestination 0x713eb0}. sizeof 0x18.
class JDestinationManager {
public:
    JDestinationManager();                                           // Win 0x69ecc0 (first part)
    virtual ~JDestinationManager() {}                                // Win 0x69e640 / 0x69e6b0
    virtual void InitDestination(j_compress_ptr) {}
    virtual boolean EmptyOutputBuffer(j_compress_ptr) { return FALSE; }
    virtual void TermDestination(j_compress_ptr) {}

    jpeg_destination_mgr m_manager;                                  // +0x04
};

// Win vtable 0x9de4f4 {sdd 0x69ea70, InitDestination 0x69ea60, ..}, ctor 0x69ecc0. sizeof 0x20.
class MemDestinationManager : public JDestinationManager {
public:
    MemDestinationManager(UInt8* data, UInt32 size) : m_data(data), m_size(size) {}
    virtual ~MemDestinationManager() {}                              // Win 0x69ea10 / 0x69ea70
    virtual void InitDestination(j_compress_ptr)                     // Win 0x69ea60
    {
        m_manager.next_output_byte = m_data;
        m_manager.free_in_buffer = m_size;
    }

    UInt8* m_data;                                                   // +0x18
    UInt32 m_size;                                                   // +0x1c
};

// Win vtable 0x9de474 {sdd 0x69e350}. +0x04 error manager, +0x08 progress manager (always 0
// here; the trampolines would pass it on). (names inferred)
class JCommon {
public:
    JCommon(JErrorManager* error, jpeg_progress_mgr* progress) : m_error(error), m_progress(progress) {}
    virtual ~JCommon() {}                                            // Win 0x69e340 / 0x69e350

    JErrorManager*     m_error;                                      // +0x04
    jpeg_progress_mgr* m_progress;                                   // +0x08
};

// Win vtable 0x9de498 {sdd 0x69e550}, ctor 0x69e6d0 thiscall(destination, error, progress),
// dtor 0x69e460 (jpeg_destroy). sizeof 0x17c.
class JCompress : public JCommon {
public:
    JCompress(JDestinationManager* destination, JErrorManager* error, jpeg_progress_mgr* progress);
    virtual ~JCompress() { jpeg_destroy(reinterpret_cast<j_common_ptr>(&m_info)); }

    // Win 0x69e4d0 (name inferred)
    void Setup(UInt32 width, UInt32 height, int quality, bool forceBaseline, int colorSpace, double gamma)
    {
        m_info.image_width = width;
        m_info.image_height = height;
        switch (colorSpace) {
        case JCS_GRAYSCALE:
            m_info.input_components = 1;
            break;
        case JCS_RGB:
        case JCS_YCbCr:
            m_info.input_components = 3;
            break;
        case JCS_CMYK:
        case JCS_YCCK:
            m_info.input_components = 4;
            break;
        default:
            break;
        }
        m_info.input_gamma = gamma;
        m_info.in_color_space = (J_COLOR_SPACE)colorSpace;
        jpeg_set_defaults(&m_info);
        jpeg_set_quality(&m_info, quality, forceBaseline ? TRUE : FALSE);
    }

    jpeg_compress_struct m_info;                                     // +0x10
    JDestinationManager* m_destination;                              // +0x178
};

// Win vtable 0x9de4a0 {sdd 0x69e620}, ctor 0x69e880 thiscall(source, error, progress), dtor
// 0x69e570 (jpeg_destroy). sizeof 0x1c4.
class JDecompress : public JCommon {
public:
    JDecompress(JSourceManager* source, JErrorManager* error, jpeg_progress_mgr* progress);
    virtual ~JDecompress() { jpeg_destroy(reinterpret_cast<j_common_ptr>(&m_info)); }

    bool StartDecompress() { return jpeg_start_decompress(&m_info) != 0; }   // Win 0x69e5e0
    bool FinishDecompress() { return jpeg_finish_decompress(&m_info) != 0; } // Win 0x69e600

    jpeg_decompress_struct m_info;                                   // +0x10
    JSourceManager*        m_source;                                 // +0x1c0
};

inline JCommon* GetCommon(j_common_ptr cinfo) { return static_cast<JCommon*>(cinfo->client_data); }

// --- libjpeg callbacks (Win 0x69e380-0x69e400, 0x69e650-0x69e690, 0x69e7b0-0x69e840) ---------
void ErrorExitCallback(j_common_ptr cinfo)              { GetCommon(cinfo)->m_error->ErrorExit(cinfo); }
void EmitMessageCallback(j_common_ptr cinfo, int level)  { GetCommon(cinfo)->m_error->EmitMessage(cinfo, level); }
void OutputMessageCallback(j_common_ptr cinfo)          { GetCommon(cinfo)->m_error->OutputMessage(cinfo); }
void FormatMessageCallback(j_common_ptr cinfo, char* b) { GetCommon(cinfo)->m_error->FormatMessage(cinfo, b); }
void ResetErrorCallback(j_common_ptr cinfo)             { GetCommon(cinfo)->m_error->ResetErrorManager(cinfo); }

inline JDecompress* GetDecompress(j_decompress_ptr cinfo)
{
    return static_cast<JDecompress*>(static_cast<JCommon*>(cinfo->client_data));
}
inline JCompress* GetCompress(j_compress_ptr cinfo)
{
    return static_cast<JCompress*>(static_cast<JCommon*>(cinfo->client_data));
}

void InitSourceCallback(j_decompress_ptr cinfo)           { GetDecompress(cinfo)->m_source->InitSource(cinfo); }
boolean FillInputBufferCallback(j_decompress_ptr cinfo)   { return GetDecompress(cinfo)->m_source->FillInputBuffer(cinfo); }
void SkipInputDataCallback(j_decompress_ptr cinfo, long n)
{
    if (n > 0)
        GetDecompress(cinfo)->m_source->SkipInputData(cinfo, n);
}
boolean ResyncCallback(j_decompress_ptr cinfo, int desired) { return GetDecompress(cinfo)->m_source->ResyncToRestart(cinfo, desired); }
void TermSourceCallback(j_decompress_ptr cinfo)           { GetDecompress(cinfo)->m_source->TermSource(cinfo); }

void InitDestinationCallback(j_compress_ptr cinfo)        { GetCompress(cinfo)->m_destination->InitDestination(cinfo); }
boolean EmptyOutputBufferCallback(j_compress_ptr cinfo)   { return GetCompress(cinfo)->m_destination->EmptyOutputBuffer(cinfo); }
void TermDestinationCallback(j_compress_ptr cinfo)        { GetCompress(cinfo)->m_destination->TermDestination(cinfo); }

JErrorManager::JErrorManager()
{
    memset(&m_manager, 0, sizeof(m_manager));
    m_manager.error_exit = ErrorExitCallback;
    m_manager.emit_message = EmitMessageCallback;
    m_manager.output_message = OutputMessageCallback;
    m_manager.format_message = FormatMessageCallback;
    m_manager.reset_error_mgr = ResetErrorCallback;
}

JSourceManager::JSourceManager()
{
    m_manager.next_input_byte = 0;
    m_manager.bytes_in_buffer = 0;
    m_manager.init_source = InitSourceCallback;
    m_manager.fill_input_buffer = FillInputBufferCallback;
    m_manager.skip_input_data = SkipInputDataCallback;
    m_manager.resync_to_restart = ResyncCallback;
    m_manager.term_source = TermSourceCallback;
}

JDestinationManager::JDestinationManager()
{
    m_manager.next_output_byte = 0;
    m_manager.free_in_buffer = 0;
    m_manager.init_destination = InitDestinationCallback;
    m_manager.empty_output_buffer = EmptyOutputBufferCallback;
    m_manager.term_destination = TermDestinationCallback;
}

// Win 0x69e6d0. err/client_data are set before jpeg_CreateCompress (which keeps them).
JCompress::JCompress(JDestinationManager* destination, JErrorManager* error, jpeg_progress_mgr* progress)
    : JCommon(error, progress), m_destination(destination)
{
    m_info.err = &m_error->m_manager;
    m_info.client_data = static_cast<JCommon*>(this);
    jpeg_CreateCompress(&m_info, JPEG_LIB_VERSION, sizeof(jpeg_compress_struct));
    m_info.dest = &m_destination->m_manager;
    if (m_progress != 0)
        m_info.progress = m_progress;
}

// Win 0x69e880
JDecompress::JDecompress(JSourceManager* source, JErrorManager* error, jpeg_progress_mgr* progress)
    : JCommon(error, progress), m_source(source)
{
    m_info.err = &m_error->m_manager;
    m_info.client_data = static_cast<JCommon*>(this);
    jpeg_CreateDecompress(&m_info, JPEG_LIB_VERSION, sizeof(jpeg_decompress_struct));
    m_info.src = &m_source->m_manager;
    if (m_progress != 0)
        m_info.progress = m_progress;
}

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

} // namespace

// Win 0x69ae40 (ICF with BMPBitmapCodec::SetStream)
void JPGBitmapCodec::SetStream(DataStream* stream)
{
    m_header = 0;
    BitmapCodecSurfaceHelper::SetStream(stream);
}

// Win 0x69eae0 (ICF with PNGBitmapCodec::GetEncodedSize)
UInt32 JPGBitmapCodec::GetEncodedSize(const Bitmap& bitmap)
{
    return bitmap.m_width * bitmap.m_height * 4 + 0x1000;
}

// Win 0x69f160
// Only JFIF files are accepted (FF D8 FF E0 xx xx "JFIF\0", more than 12 bytes); one component
// -> 0x80000108 (grey palette made by Decode), three -> 0x218. Surface::m_dataSize = stream size.
RefPointer<BitmapCodec::Header> JPGBitmapCodec::ReadHeader()
{
    if (!m_header.IsNull())
        return m_header.Get();
    if (m_stream.IsNull() || m_stream->GetBuffer() == 0 || m_stream->GetBufferSize() <= 12)
        return 0;
    const UInt8* data = m_stream->GetBuffer();
    if (data[0] != 0xff || data[1] != 0xd8 || data[2] != 0xff || data[3] != 0xe0 ||
        data[6] != 'J' || data[7] != 'F' || data[8] != 'I' || data[9] != 'F' || data[10] != 0)
        return 0;

    RefPointer<JPEGHeader> header = new JPEGHeader;
    Surface surface;
    surface.m_width = 0;
    surface.m_height = 0;
    surface.m_format = 0;
    surface.m_dataSize = m_stream->GetBufferSize();
    try {
        JErrorManager error;
        MemSourceManager source(data, m_stream->GetBufferSize());
        JDecompress decompress(&source, &error, 0);
        if (jpeg_read_header(&decompress.m_info, TRUE) != JPEG_HEADER_OK)
            return 0;
        surface.m_width = decompress.m_info.image_width;
        surface.m_height = decompress.m_info.image_height;
        if (surface.m_width == 0 || surface.m_height == 0)
            return 0;
        if (decompress.m_info.num_components == 1)
            surface.m_format = 0x80000108;
        else if (decompress.m_info.num_components == 3)
            surface.m_format = Bitmap::SwapRedBlueFormat(0x118);
        else
            return 0;
    } catch (...) {
        return 0;
    }
    SurfaceArray levels;
    levels.PushBack(surface);
    header->m_surfaces.PushBack(levels);
    m_header = header;
    return header.Get();
}

// Win 0x69f460
bool JPGBitmapCodec::Decode(Bitmap& bitmap, const Color* colorKey)
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
    try {
        JErrorManager error;
        MemSourceManager source(m_stream->GetBuffer(), m_stream->GetBufferSize());
        JDecompress decompress(&source, &error, 0);
        jpeg_decompress_struct& info = decompress.m_info;
        if (jpeg_read_header(&info, TRUE) != JPEG_HEADER_OK || !decompress.StartDecompress())
            return false;
        UInt32 rowBytes = info.output_width * info.output_components;
        ScopedBuffer pixels(rowBytes * height);
        BitmapPixelSource pixelSource(0, 0, 0, 0);
        if (info.num_components == 1) {
            m_palette.Resize(0x100, Color());
            for (int i = 0; i < 0x100; ++i)
                m_palette.Begin()[i] = Color::FromBytes((UInt8)i, (UInt8)i, (UInt8)i, 0xff);
            pixelSource.Set(surface.m_format, pixels.Get(), (Int32)rowBytes, m_palette.Begin());
        } else if (info.num_components == 3) {
            pixelSource.Set(surface.m_format, pixels.Get(), (Int32)rowBytes, 0);
        }
        JSAMPROW row = pixels.Get();
        while (info.output_scanline < info.output_height) {
            JSAMPROW rows[1] = { row };
            if (jpeg_read_scanlines(&info, rows, 1) == 0)
                return false;
            row += rowBytes;
        }
        decompress.FinishDecompress();
        if (info.output_scanline != info.output_height)
            return false;
        (void)colorKey;     // NOTE: the colour key is ignored by the JPEG decoder
        BitmapPixelDestination destination(bitmap.m_format, bitmap.m_data, bitmap.m_pitch,
                                           (Int32)bitmap.m_format < 0 ? m_palette.Begin() : 0);
        BitmapPixelConverter converter(&pixelSource, &destination);
        converter.Convert(width, height);
        return true;
    } catch (...) {
        return false;
    }
}

// Win 0x69ed80
// Indexed input is refused. RGB (0x218 rows) JFIF with libjpeg defaults at the requested
// quality (JPGBitmapCodec::EncodeArgs, default 100), written into the stream's buffer.
bool JPGBitmapCodec::Encode(const Bitmap& bitmap, const BitmapCodec::EncodeArgs* args)
{
    if (bitmap.m_data == 0 || bitmap.m_pitch == 0 || bitmap.m_format == 0 || m_stream.IsNull() ||
        m_stream->GetBuffer() == 0 || m_stream->GetBufferSize() == 0 || (Int32)bitmap.m_format < 0)
        return false;
    UInt32 width = bitmap.m_width;
    UInt32 height = bitmap.m_height;
    try {
        JErrorManager error;
        UInt32 fileFormat = Bitmap::SwapRedBlueFormat(0x118);
        m_header = 0;
        UInt32 rowBytes = width * 3;
        ScopedBuffer pixels(rowBytes * height);
        BitmapPixelSource source(bitmap.m_format, bitmap.m_data, bitmap.m_pitch, 0);
        BitmapPixelDestination destination(fileFormat, pixels.Get(), (Int32)rowBytes, 0);
        BitmapPixelConverter converter(&source, &destination);
        converter.Convert(width, height);

        UInt8* buffer = m_stream->GetBuffer();
        int quality = 100;
        const JPGBitmapCodec::EncodeArgs* options = dynamic_cast<const JPGBitmapCodec::EncodeArgs*>(args);
        if (options != 0 && options->m_quality < 0x65)
            quality = (int)options->m_quality;
        MemDestinationManager output(buffer, m_stream->GetBufferSize());
        JCompress compress(&output, &error, 0);
        compress.Setup(width, height, quality, false, JCS_RGB, 1.0);
        jpeg_start_compress(&compress.m_info, TRUE);
        JSAMPROW row = pixels.Get();
        while (compress.m_info.next_scanline < compress.m_info.image_height) {
            JSAMPROW rows[1] = { row };
            if (jpeg_write_scanlines(&compress.m_info, rows, 1) == 0)
                return false;
            row += rowBytes;
        }
        jpeg_finish_compress(&compress.m_info);
        m_stream->SetBufferSize((UInt32)(output.m_manager.next_output_byte - buffer));
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace LEGO

#endif // _WIN32
