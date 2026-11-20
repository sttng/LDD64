// LEGO Digital Designer — restored source
// Mac image I/O through ImageIO / CoreGraphics (Mac branch only); the Mac registers only the BMP
// codec (Mac 0x2a6a50). Reading: inline in OpenGLTextureFactory::CreateObject (Mac 0x226870,
// block 0x226aeb-0x226c78), here DecodeImageWithImageIO (name inferred). Writing: Mac 0x1492b0
// (TU 063) encodes PNG into a DataStreamWithDataRef (Mac 0x14e450-0x14e5bf) for the HTML / BOM
// export and the model upload.
//
// The ImageIO code is restored under __APPLE__. The Linux-hosted Mac branch has no ImageIO:
// there a HOST-ONLY fallback (marked; NOT restored code) decodes PNG / JPEG with the thirdparty
// libpng / libjpeg and BMP with the restored BMPBitmapCodec, and
// writes PNG with libpng. Differences of the fallback: no colour management
// (ImageIO converts into the AdobeRGB1998 context colour space; the fallback keeps the file's
// values), premultiplication rounds as (c * a + 127) / 255, other ImageIO formats (GIF, TIFF, ...)
// are not decoded, and the written PNG carries no colour profile.
#pragma once
#include "LEGO/Types.h"
#include "Bitmap.h"
#include "DataStream.h"
#include "Object.h"

#if !defined(_WIN32)

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace LEGO {

// Larger images are refused by the texture factory (Mac 0x226b3c: width and height <= 0x400;
// trace "Images should not be larger than 1024 pixels: " << path).
enum { kMaxImageIOImageSize = 0x400 };

// Mac 0x226aeb-0x226c78 (inlined in OpenGLTextureFactory::CreateObject).
// Decodes the image file `data` / `size` into `bitmap`: format 0x40000220 (BGRA, alpha
// premultiplied: CGBitmapContext with kCGImageAlphaPremultipliedFirst |
// kCGBitmapByteOrder32Little), pitch width * 4, data from new[] (zeroed, then drawn into; the
// caller owns it), depth/palette 0. Returns false (bitmap untouched) when the image is wider or
// taller than kMaxImageIOImageSize.
// NOTE: an undecodable file is not an error: CGImageSourceCreateImageAtIndex returns null, its
// size reads as 0 x 0, and the texture factory goes on with an empty bitmap (true, 0 x 0, a
// new[0] block).
bool DecodeImageWithImageIO(const void* data, UInt32 size, Bitmap& bitmap);

// Mac 0x14e450-0x14e5bf. DataStreamInterface over a CFMutableData (sizeof 0x10: vptr, CFDataRef
// +4, Object +8; vtables 0x766e14 / 0x766e3c). Releases the data with the stream.
class DataStreamWithDataRef : public DataStreamInterface {
public:
#if defined(__APPLE__)
    explicit DataStreamWithDataRef(CFMutableDataRef data) : m_data(data) {}   // inlined (Mac 0x1492c6)
    virtual ~DataStreamWithDataRef() { CFRelease(m_data); }          // Mac 0x14e490 (D1), 0x14e500 (D0)
    virtual const void* GetData() const { return CFDataGetBytePtr(m_data); }    // Mac 0x14e450
    virtual UInt32 GetSize() const { return (UInt32)CFDataGetLength(m_data); }  // Mac 0x14e470
    CFMutableDataRef GetDataRef() const { return m_data; }

private:
    CFMutableDataRef m_data;   // +0x04
#else
    // HOST-ONLY (not restored code): a growable heap block in place of the CFMutableData.
    DataStreamWithDataRef() : m_bytes(0), m_size(0), m_capacity(0) {}
    virtual ~DataStreamWithDataRef() { delete[] m_bytes; }
    virtual const void* GetData() const { return m_bytes; }
    virtual UInt32 GetSize() const { return m_size; }
    void Append(const void* bytes, UInt32 count);

private:
    UInt8* m_bytes;
    UInt32 m_size;
    UInt32 m_capacity;
#endif

    DataStreamWithDataRef(const DataStreamWithDataRef&);
    DataStreamWithDataRef& operator=(const DataStreamWithDataRef&);
};

// Mac 0x1492b0 (name inferred). PNG file of `bitmap` (read as premultiplied BGRA: width +0,
// height +4, pitch +0x14, data +0x18; colour space GenericRGB), or null when
// CGImageDestinationFinalize fails.
RefPointer<DataStreamInterface> EncodePNGWithImageIO(const Bitmap& bitmap);

} // namespace LEGO

#endif // !_WIN32
