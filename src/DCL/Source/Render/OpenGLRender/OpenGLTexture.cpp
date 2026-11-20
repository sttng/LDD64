// LEGO Digital Designer — restored source
// OpenGL textures — see OpenGLTexture.h. TU 036 "OpenGLTexture.cpp" (Mac 0xf87f0-0xfdeaf,
// Win 0x4f5490-0x4f8e80, 0x5014b0-0x502b80). The mutable classes' other methods and the
// texture factory are in OpenGLTextureFactory.cpp (TU 168).
//
// Pixel formats (BitmapFormat → internal format / format / type / bytes per pixel): the Mac
// inlines three tables, transcribed as three functions: image textures (2D 0xf8ba0, cube
// 0xfa8f0, 3D 0xfb620; 24/32-bit may be compressed), mutable textures (0xf9160, 0xfb020; no
// compression, some formats refused), render targets (0xf9790, 0xf9f40; no data, format/type of
// 0x118/0x218 and 0x40000220/0x40000620 swapped vs. images, harmless).
#include "OpenGLTexture.h"
#include "OpenGLRender.h"
#include "OpenGLCrashReport.h"
#include "OpenGLTextureFactory.h"
#include "LEGO/String.h"

#if defined(_WIN32)
#include <GL/glu.h>
#else
#include <OpenGL/glu.h>
#endif
#include <string.h>

namespace LEGO {

namespace {

struct GLPixelFormat {
    UInt32 internalFormat;
    UInt32 format;
    UInt32 type;
    UInt32 bytesPerPixel;
};

#if !defined(_WIN32)
// Image table (Mac 0xf8ba0 / 0xfa8f0 / 0xfb620). `alphaMode` is stored in the texture except for
// the RGB/BGR byte formats (0). Compression: ARB_texture_compression and mipmapMode 2 or 3.
bool GetImageFormatMac(UInt32 bitmapFormat, UInt32 alphaMode, UInt32 mipmapMode, bool compression,
                       GLPixelFormat& out, UInt32& textureFormat)
{
    bool compress = compression && (mipmapMode | 1) == 3;
    switch (bitmapFormat) {
    case kBitmapFormatIndexed8:
        out.internalFormat = GL_RGBA8; out.format = GL_LUMINANCE; out.type = GL_UNSIGNED_BYTE; out.bytesPerPixel = 1;
        textureFormat = alphaMode;
        return true;
    case 8:
        out.internalFormat = GL_LUMINANCE8; out.format = GL_LUMINANCE; out.type = GL_UNSIGNED_BYTE; out.bytesPerPixel = 1;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatBGR24:
        out.internalFormat = GL_RGB8; out.format = GL_BGR; out.type = GL_UNSIGNED_BYTE; out.bytesPerPixel = 3;
        textureFormat = 0;
        return true;
    case kBitmapFormatRGB24:
        out.internalFormat = compress ? GL_COMPRESSED_RGB_ARB : GL_RGB8;
        out.format = GL_RGB; out.type = GL_UNSIGNED_BYTE; out.bytesPerPixel = 3;
        textureFormat = 0;
        return true;
    case 0x230:   // RGB, 16-bit float channels
        out.internalFormat = GL_RGB16F_ARB; out.format = GL_RGB; out.type = GL_FLOAT; out.bytesPerPixel = 3;
        textureFormat = alphaMode;
        return true;
    case 0x260:   // RGB, 32-bit float channels
        out.internalFormat = GL_RGB32F_ARB; out.format = GL_RGB; out.type = GL_FLOAT; out.bytesPerPixel = 3;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatBGRA32:
        out.internalFormat = GL_RGBA8; out.format = GL_BGRA; out.type = GL_UNSIGNED_BYTE; out.bytesPerPixel = 4;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatARGB32:
        out.internalFormat = GL_RGBA8; out.format = GL_BGRA; out.type = GL_UNSIGNED_INT_8_8_8_8_REV;
        out.bytesPerPixel = 4;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatRGBA32:
        out.internalFormat = compress ? GL_COMPRESSED_RGBA_ARB : GL_RGBA8;
        out.format = GL_RGBA; out.type = GL_UNSIGNED_BYTE; out.bytesPerPixel = 4;
        textureFormat = alphaMode;
        return true;
    case 0x40000840:   // RGBA, 16-bit float channels
        out.internalFormat = GL_RGBA16F_ARB; out.format = GL_RGBA; out.type = GL_FLOAT; out.bytesPerPixel = 4;
        textureFormat = alphaMode;
        return true;
    case 0x40000880:   // RGBA, 32-bit float channels
        out.internalFormat = GL_RGBA32F_ARB; out.format = GL_RGBA; out.type = GL_FLOAT; out.bytesPerPixel = 4;
        textureFormat = alphaMode;
        return true;
    default:
        return false;
    }
}

// Mutable table (Mac 0xf9160 / 0xfb020). The type is written first; refused formats leave
// GL_UNSIGNED_BYTE in it.
bool GetMutableFormatMac(UInt32 bitmapFormat, UInt32 alphaMode, GLPixelFormat& out, UInt32& textureFormat)
{
    out.type = GL_UNSIGNED_BYTE;
    switch (bitmapFormat) {
    case kBitmapFormatBGRA32:
        out.format = GL_BGRA; out.internalFormat = GL_RGBA8; out.bytesPerPixel = 4;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatARGB32:
        out.format = GL_BGRA; out.type = GL_UNSIGNED_INT_8_8_8_8_REV; out.internalFormat = GL_RGBA8;
        out.bytesPerPixel = 4;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatRGBA32:
        out.format = GL_RGBA; out.internalFormat = GL_RGBA8; out.bytesPerPixel = 4;
        textureFormat = alphaMode;
        return true;
    case 0x40000840:
        out.format = GL_RGBA; out.type = GL_FLOAT; out.internalFormat = GL_RGBA16F_ARB; out.bytesPerPixel = 4;
        textureFormat = alphaMode;
        return true;
    case 0x40000880:
        out.format = GL_RGBA; out.type = GL_FLOAT; out.internalFormat = GL_RGBA32F_ARB; out.bytesPerPixel = 4;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatIndexed8:
        out.format = GL_LUMINANCE; out.internalFormat = GL_RGBA8; out.bytesPerPixel = 1;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatBGR24:
        out.format = GL_BGR; out.internalFormat = GL_RGB8; out.bytesPerPixel = 3;
        textureFormat = 0;
        return true;
    case kBitmapFormatRGB24:
        out.format = GL_RGB; out.internalFormat = GL_RGB8; out.bytesPerPixel = 3;
        textureFormat = 0;
        return true;
    case 0x230:
        out.format = GL_RGB; out.type = GL_FLOAT; out.internalFormat = GL_RGB16F_ARB; out.bytesPerPixel = 3;
        textureFormat = alphaMode;
        return true;
    case 0x260:
        out.format = GL_RGB; out.type = GL_FLOAT; out.internalFormat = GL_RGB32F_ARB; out.bytesPerPixel = 3;
        textureFormat = alphaMode;
        return true;
    case 8:
        out.format = GL_LUMINANCE; out.internalFormat = GL_LUMINANCE8; out.bytesPerPixel = 1;
        textureFormat = alphaMode;
        return true;
    default:
        // explicitly refused: 0x120, 0x130, 0x160, 0x320, 0x520, 0x720, 0x40000420, 0x40000440,
        // 0x40000480; anything else unknown
        return false;
    }
}

// Render-target table (Mac 0xf9790 / 0xf9f40).
bool GetRenderTargetFormatMac(UInt32 bitmapFormat, UInt32 alphaMode, GLPixelFormat& out,
                              UInt32& textureFormat)
{
    out.bytesPerPixel = 0;
    switch (bitmapFormat) {
    case kBitmapFormatIndexed8:
        out.type = GL_UNSIGNED_BYTE; out.format = GL_LUMINANCE; out.internalFormat = GL_RGBA8;
        textureFormat = alphaMode;
        return true;
    case 8:
        out.type = GL_UNSIGNED_BYTE; out.format = GL_LUMINANCE; out.internalFormat = GL_LUMINANCE8;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatBGR24:
        out.format = GL_RGB; out.type = GL_UNSIGNED_BYTE; out.internalFormat = GL_RGB8;
        textureFormat = 0;
        return true;
    case 0x130:
        out.type = GL_FLOAT; out.format = GL_RGB; out.internalFormat = GL_RGB16F_ARB;
        textureFormat = alphaMode;
        return true;
    case 0x160:
        out.type = GL_FLOAT; out.format = GL_RGB; out.internalFormat = GL_RGB32F_ARB;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatRGB24:
        out.format = GL_BGR; out.type = GL_UNSIGNED_BYTE; out.internalFormat = GL_RGB8;
        textureFormat = 0;
        return true;
    case kBitmapFormatBGRA32:
        out.type = GL_UNSIGNED_INT_8_8_8_8_REV; out.format = GL_BGRA; out.internalFormat = GL_RGBA8;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatABGR32:
        out.type = GL_UNSIGNED_BYTE; out.format = GL_RGBA; out.internalFormat = GL_RGBA8;
        textureFormat = alphaMode;
        return true;
    case 0x40000440:
        out.type = GL_FLOAT; out.format = GL_RGBA; out.internalFormat = GL_RGBA16F_ARB;
        textureFormat = alphaMode;
        return true;
    case 0x40000480:
        out.type = GL_FLOAT; out.format = GL_RGBA; out.internalFormat = GL_RGBA32F_ARB;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatARGB32:
        out.type = GL_UNSIGNED_BYTE; out.format = GL_BGRA; out.internalFormat = GL_RGBA8;
        textureFormat = alphaMode;
        return true;
    default:
        return false;
    }
}

#endif // !_WIN32

#if defined(_WIN32)
// Win: one table (Win 0x501af0) for every initialiser, looked up with the red/blue-swapped
// format (Bitmap::SwapRedBlueFormat, Win 0x4f5310). The image and mutable initialisers pass the
// format (the helper swaps it); the render-target ones swap it first (0x5023f4 / 0x502774), so
// their lookup uses the unswapped code. The type is GL_UNSIGNED_BYTE unless set; compression
// for mipmap modes 2 and 3 (the mutable and render-target callers pass 0).
// Differences to the Mac tables (NOTE): 0x40000620 (ARGB) is refused by the image and mutable
// initialisers, 0x40000220 (BGRA) by the render-target ones, and render targets of 0x118 / 0x218
// get GL_BGR / GL_RGB like images.
bool WinFormatTable(UInt32 swapped, UInt32 alphaMode, UInt32 mipmapMode, bool compression,
                    GLPixelFormat& out, UInt32& textureFormat)
{
    bool compress = compression && (mipmapMode == 3 || mipmapMode == 2);
    out.type = GL_UNSIGNED_BYTE;
    switch (swapped) {
    case 0x118:          // RGB24
        out.internalFormat = compress ? GL_COMPRESSED_RGB_ARB : GL_RGB8;
        out.format = GL_RGB; out.bytesPerPixel = 3;
        textureFormat = 0;
        return true;
    case 8:
        out.format = GL_LUMINANCE; out.bytesPerPixel = 1; out.internalFormat = GL_LUMINANCE8;
        textureFormat = alphaMode;
        return true;
    case 0x130:          // 0x230: RGB, 16-bit float
        out.format = GL_RGB; out.type = GL_FLOAT; out.bytesPerPixel = 3; out.internalFormat = GL_RGB16F_ARB;
        textureFormat = alphaMode;
        return true;
    case 0x160:          // 0x260: RGB, 32-bit float
        out.format = GL_RGB; out.type = GL_FLOAT; out.bytesPerPixel = 3; out.internalFormat = GL_RGB32F_ARB;
        textureFormat = alphaMode;
        return true;
    case 0x218:          // BGR24
        out.format = GL_BGR; out.bytesPerPixel = 3; out.internalFormat = GL_RGB8;
        textureFormat = 0;
        return true;
    case 0x40000420:     // RGBA32
        out.internalFormat = compress ? GL_COMPRESSED_RGBA_ARB : GL_RGBA8;
        out.format = GL_RGBA; out.bytesPerPixel = 4;
        textureFormat = alphaMode;
        return true;
    case 0x40000440:     // 0x40000840: RGBA, 16-bit float
        out.format = GL_RGBA; out.type = GL_FLOAT; out.bytesPerPixel = 4; out.internalFormat = GL_RGBA16F_ARB;
        textureFormat = alphaMode;
        return true;
    case 0x40000480:     // 0x40000880: RGBA, 32-bit float
        out.format = GL_RGBA; out.type = GL_FLOAT; out.bytesPerPixel = 4; out.internalFormat = GL_RGBA32F_ARB;
        textureFormat = alphaMode;
        return true;
    case 0x40000620:     // BGRA32
        out.format = GL_BGRA; out.bytesPerPixel = 4; out.internalFormat = GL_RGBA8;
        textureFormat = alphaMode;
        return true;
    case kBitmapFormatIndexed8:
        out.format = GL_LUMINANCE; out.bytesPerPixel = 1; out.internalFormat = GL_RGBA8;
        textureFormat = alphaMode;
        return true;
    default:
        return false;
    }
}

bool GetImageFormat(UInt32 bitmapFormat, UInt32 alphaMode, UInt32 mipmapMode, bool compression,
                    GLPixelFormat& out, UInt32& textureFormat)
{
    return WinFormatTable(Bitmap::SwapRedBlueFormat(bitmapFormat), alphaMode, mipmapMode, compression, out,
                          textureFormat);
}

bool GetMutableFormat(UInt32 bitmapFormat, UInt32 alphaMode, GLPixelFormat& out, UInt32& textureFormat)
{
    return WinFormatTable(Bitmap::SwapRedBlueFormat(bitmapFormat), alphaMode, 0, false, out, textureFormat);
}

bool GetRenderTargetFormat(UInt32 bitmapFormat, UInt32 alphaMode, GLPixelFormat& out, UInt32& textureFormat)
{
    return WinFormatTable(Bitmap::SwapRedBlueFormat(Bitmap::SwapRedBlueFormat(bitmapFormat)), alphaMode, 0,
                          false, out, textureFormat);
}
#else
bool GetImageFormat(UInt32 bitmapFormat, UInt32 alphaMode, UInt32 mipmapMode, bool compression,
                    GLPixelFormat& out, UInt32& textureFormat)
{
    return GetImageFormatMac(bitmapFormat, alphaMode, mipmapMode, compression, out, textureFormat);
}

bool GetMutableFormat(UInt32 bitmapFormat, UInt32 alphaMode, GLPixelFormat& out, UInt32& textureFormat)
{
    return GetMutableFormatMac(bitmapFormat, alphaMode, out, textureFormat);
}

bool GetRenderTargetFormat(UInt32 bitmapFormat, UInt32 alphaMode, GLPixelFormat& out, UInt32& textureFormat)
{
    return GetRenderTargetFormatMac(bitmapFormat, alphaMode, out, textureFormat);
}
#endif

// The wrap parameter of every initialiser.
GLint WrapParameter(UInt32 wrapMode)
{
    if (wrapMode == Texture::kWrapClampToEdge)
        return GL_CLAMP_TO_EDGE;
    if (wrapMode == Texture::kWrapMirroredRepeat)
        return GL_MIRRORED_REPEAT_ARB;
    return GL_REPEAT;
}

// glGenTextures + glPrioritizeTextures + glBindTexture (inlined in every initialiser).
void GenerateTexture(UInt32& name, UInt32 target, Real32 priority)
{
    glGenTextures(1, (GLuint*)&name);
    glPrioritizeTextures(1, (const GLuint*)&name, &priority);
    glBindTexture(target, name);
    NoteGLStateChanged();
}

} // namespace

// ===========================================================================================
// OpenGLTexture
// ===========================================================================================

// Mac 0x227a00 (inlined into the subclass ctors), Win 0x4f5490 area
OpenGLTexture::OpenGLTexture(OpenGLRender* render)
    : m_textureID((UInt32)-1), m_target(0), m_format(0), m_size(), m_imageSize(), m_render(render)
{
    m_render->RegisterTexture(this);
}

// Mac 0xfdb60 (D2)
OpenGLTexture::~OpenGLTexture()
{
    m_render->UnregisterTexture(this);
}

UInt32  OpenGLTexture::GetFormat() const    { return m_format; }       // Mac 0xfbea0, Win 0x5005f0
Vector2 OpenGLTexture::GetSize() const      { return m_size; }         // Mac 0xfbeb0, Win 0x4f5600
Vector2 OpenGLTexture::GetImageSize() const { return m_imageSize; }    // Mac 0xfbee0, Win 0x4f5620

// Mac 0xfbf10, Win 0x40be80 (ICF)
void OpenGLTexture::NotifyChanged(UInt32)
{
}

// Mac 0xfbf20, Win 0x4f5640
UInt32 OpenGLTexture::GetTextureName(UInt32) const
{
    return m_textureID;
}

// Mac 0xf9f00, Win 0x4f5650
void OpenGLTexture::ReleaseGLObject()
{
    GLuint name = m_textureID;
    if (name != (GLuint)-1) {
        glDeleteTextures(1, &name);
        NoteGLStateChanged();
        m_textureID = (UInt32)-1;
    }
}

// Mac 0xf9020
void OpenGLTexture::OnFinalRelease()
{
    m_render->MakeCurrent();
    LDD_GL_TRY
    ReleaseGLObject();
    Notifier<TextureObserver>::OnFinalRelease();   // detaches the observers (inlined, Mac 0xfc120)
    LDD_GL_CATCH
}

// The observer loop of Mac 0xfc9d0 / 0xfcd90 / 0xfd140 / 0x227d40 / 0x2283a0.
void OpenGLTexture::NotifyObservers(UInt32 face)
{
    Iterator it(*this);
    while (TextureObserver* observer = it.Next(m_observers))
        observer->OnTextureChanged(face);
}

// Mac 0xf87f0
void OpenGLTexture::CreateGLTexture(Real32 priority, UInt32 wrapMode, UInt32 mipmapMode, UInt32 filter)
{
    GenerateTexture(m_textureID, m_target, priority);
    GLint wrap = WrapParameter(wrapMode);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, wrap);
    GLint magnification = filter == Texture::kFilterLinear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, magnification);
    GLint minification = magnification;
    if (mipmapMode - 1 < 2)
        minification = filter == Texture::kFilterLinear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, minification);
}

UInt32 OpenGLTexture::NextPowerOfTwo(UInt32 value)
{
    UInt32 power = 1;
    UInt32 previous;
    do {
        previous = power;
        power = previous * 2;
    } while (previous < value);
    return previous;
}

void OpenGLTexture::SetSizes(UInt32 width, UInt32 height)
{
    m_size.x = (Real32)width;
    m_size.y = (Real32)height;
    m_imageSize.x = (Real32)NextPowerOfTwo(width);
    m_imageSize.y = (Real32)NextPowerOfTwo(height);
}

void OpenGLTexture::ClearGLErrors()
{
    GLenum error;
    do {
        error = glGetError();
        if (error == GL_INVALID_OPERATION)
            break;
    } while (error != GL_NO_ERROR);
}

// Mac 0xfd680, Win 0x5014b0
bool OpenGLTexture::CheckGLError()
{
    bool ok = true;
    GLenum error = glGetError();
    while (error != GL_NO_ERROR && error != GL_INVALID_OPERATION) {
        // NOTE: the message is built and dropped (no Trace call in either build).
        String message;
        message << "glGetError returned: " << (UInt32)error << " : "
                << String((const char*)gluErrorString(error));
        ok = false;
        error = glGetError();
    }
    return ok;
}

// Mac 0xf88e0, Win 0x501d20
// NOTE: `bytesPerPixel` is the channel count for the float formats (3 / 4 instead of 12 / 16), so
// the padding copy of a non-power-of-two float image is too small and the row copies overrun it
// (heap corruption in both builds; no caller passes such images).
bool OpenGLTexture::UploadImage(const Bitmap& bitmap, UInt32 width, UInt32 height, UInt32 imageTarget,
                                UInt32 mipmapMode, UInt32 bytesPerPixel, UInt32 internalFormat,
                                UInt32 format, UInt32 type)
{
    const void* pixels;
    UInt8* copy = 0;
    if (bitmap.m_width == width && bitmap.m_height == height) {
        pixels = bitmap.m_data;
    } else {
        copy = new UInt8[height * width * bytesPerPixel];
        memset(copy, 0, height * width * bytesPerPixel);   // fixed: zeroed (low)
        const UInt8* source = static_cast<const UInt8*>(bitmap.m_data);
        UInt8* row = copy;
        for (UInt32 y = 0; y < bitmap.m_height; ++y) {
            memcpy(row, source, bitmap.m_pitch);
            source += bitmap.m_pitch;
            row += bytesPerPixel * width;
        }
        pixels = copy;
    }
    OpenGLUnpackAlignment1 unpackAlignment1;   // OpenGLTexture.h
    bool ok;
    if (mipmapMode - 1 < 2) {
        ok = gluBuild2DMipmaps(imageTarget, internalFormat, width, height, format, type, pixels) == 0;
    } else {
        ClearGLErrors();
        glTexImage2D(imageTarget, 0, internalFormat, width, height, 0, format, type, pixels);
        ok = CheckGLError();
    }
    if (copy)
        delete[] copy;
    return ok;
}

// Mac 0xf8ab0. NOTE: when the sizes differ the uploaded block is a fresh, uninitialised
// allocation (the bitmap is not copied): mutable textures fill it later.
bool OpenGLTexture::UploadImageLevel0(const Bitmap& bitmap, UInt32 width, UInt32 height,
                                      UInt32 imageTarget, UInt32 bytesPerPixel,
                                      UInt32 internalFormat, UInt32 format, UInt32 type)
{
    const void* pixels;
    UInt8* block = 0;
    if (bitmap.m_width == width && bitmap.m_height == height) {
        pixels = bitmap.m_data;
    } else {
        block = new UInt8[height * width * bytesPerPixel];
        memset(block, 0, height * width * bytesPerPixel);   // fixed: zeroed (low)
        pixels = block;
    }
    ClearGLErrors();
    glTexImage2D(imageTarget, 0, internalFormat, width, height, 0, format, type, pixels);
    bool ok = CheckGLError();
    if (block)
        delete[] block;
    return ok;
}

// ===========================================================================================
// OpenGLTexture2D
// ===========================================================================================

// Mac 0x227400, Win 0x4f58a0
OpenGLTexture2D::OpenGLTexture2D(OpenGLRender* render)
    : OpenGLTexture(render), m_path(), m_factory(0)
{
}

UInt32 OpenGLTexture2D::GetType() const { return kType2D; }           // Mac 0xfc7e0, Win 0x51e780
bool   OpenGLTexture2D::IsMutable() const { return false; }           // Mac 0xfc7f0, Win 0x4aede0
RefPointer<BitmapObject> OpenGLTexture2D::GetBitmap(UInt32) const     // Mac 0xfc800, Win 0x4f5980
{
    return RefPointer<BitmapObject>();
}

// Mac 0xf8ba0, Win 0x501fc0
bool OpenGLTexture2D::Init(const Texture::CreateArgs2D& args)
{
    LDD_GL_TRY
    GLPixelFormat pf;
    if (!GetImageFormat(args.bitmap.m_format, args.alphaMode, args.mipmapMode,
                        m_render->GetExtensions().m_textureCompression, pf, m_format))
        return false;
    m_target = GL_TEXTURE_2D;
    CreateGLTexture(args.priority, args.wrapMode, args.mipmapMode, args.filter);
    SetSizes(args.bitmap.m_width, args.bitmap.m_height);
    bool ok = UploadImage(args.bitmap, (UInt32)m_imageSize.x, (UInt32)m_imageSize.y, m_target,
                          args.mipmapMode, pf.bytesPerPixel, pf.internalFormat, pf.format, pf.type);
    if (ok)
        glBindTexture(m_target, 0);
    NoteGLStateChanged();
    return ok;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xf8fa0 (thunk 0xf90e0)
void OpenGLTexture2D::OnFinalRelease()
{
    if (m_factory)
        m_factory->RemoveTexture(m_path);
    OpenGLTexture::OnFinalRelease();
}

// ===========================================================================================
// OpenGLTexture3D
// ===========================================================================================

// Mac 0x2274e0, Win 0x4f6760
OpenGLTexture3D::OpenGLTexture3D(OpenGLRender* render)
    : OpenGLTexture(render), m_path(), m_factory(0), m_depth(0.0f), m_imageDepth(0.0f)
{
}

UInt32 OpenGLTexture3D::GetType() const { return kType3D; }           // Mac 0xfd570, Win 0x51e7d0
bool   OpenGLTexture3D::IsMutable() const { return false; }           // Mac 0xfd580
RefPointer<BitmapObject> OpenGLTexture3D::GetBitmap(UInt32) const     // Mac 0xfd590
{
    return RefPointer<BitmapObject>();
}

// Mac 0xfbb50 (inlined into the 3D initialiser's call): the 3D form of UploadImage.
// NOTE: the copy advances one destination row per source row across all slices, i.e. slices
// are packed bitmap.m_height rows apart, not `height` rows.
static bool UploadImage3D(const Bitmap& bitmap, UInt32 width, UInt32 height, UInt32 depth,
                          UInt32 target, UInt32 bytesPerPixel, UInt32 internalFormat,
                          UInt32 format, UInt32 type)
{
    const void* pixels;
    UInt8* copy = 0;
    if (bitmap.m_width == width && bitmap.m_height == height && bitmap.m_depth == depth) {
        pixels = bitmap.m_data;
    } else {
        copy = new UInt8[height * width * depth * bytesPerPixel];
        const UInt8* source = static_cast<const UInt8*>(bitmap.m_data);
        UInt8* row = copy;
        for (UInt32 z = 0; z < bitmap.m_depth; ++z) {
            for (UInt32 y = 0; y < bitmap.m_height; ++y) {
                memcpy(row, source, bitmap.m_pitch);
                source += bitmap.m_pitch;
                row += bytesPerPixel * width;
            }
        }
        pixels = copy;
    }
    OpenGLUnpackAlignment1 unpackAlignment1;   // OpenGLTexture.h
    GLenum error;
    do {
        error = glGetError();
        if (error == GL_INVALID_OPERATION)
            break;
    } while (error != GL_NO_ERROR);
    glTexImage3D(target, 0, internalFormat, width, height, depth, 0, format, type, pixels);
    // CheckGLError (Mac 0xfd680)
    bool ok = true;
    error = glGetError();
    while (error != GL_NO_ERROR && error != GL_INVALID_OPERATION) {
        ok = false;
        error = glGetError();
    }
    if (copy)
        delete[] copy;
    return ok;
}

// Mac 0xfb620, Win 0x503070. NOTE: GL_EXT_texture3D support is not checked.
bool OpenGLTexture3D::Init(const Texture::CreateArgs3D& args)
{
    LDD_GL_TRY
    GLPixelFormat pf;
    if (!GetImageFormat(args.bitmap.m_format, args.alphaMode, args.mipmapMode,
                        m_render->GetExtensions().m_textureCompression, pf, m_format))
        return false;
    m_target = GL_TEXTURE_3D;
    GenerateTexture(m_textureID, m_target, args.priority);
    GLint wrap = WrapParameter(args.wrapMode);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_R, wrap);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    SetSizes(args.width, args.height);
    m_depth = (Real32)args.depth;
    m_imageDepth = (Real32)NextPowerOfTwo(args.depth);
    bool ok = UploadImage3D(args.bitmap, (UInt32)m_imageSize.x, (UInt32)m_imageSize.y,
                            (UInt32)m_imageDepth, m_target, pf.bytesPerPixel, pf.internalFormat,
                            pf.format, pf.type);
    if (ok)
        glBindTexture(m_target, 0);
    NoteGLStateChanged();
    return ok;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xfbcd0 (thunk 0xfbd50): same body as OpenGLTexture2D's.
void OpenGLTexture3D::OnFinalRelease()
{
    if (m_factory)
        m_factory->RemoveTexture(m_path);
    OpenGLTexture::OnFinalRelease();
}

// ===========================================================================================
// OpenGLTextureCube (Init only; the accessors are in TU 168)
// ===========================================================================================

// Mac 0xfa8f0, Win 0x502bf0. Needs ARB_texture_cube_map. Faces +X, -X, +Y, -Y, +Z, -Z, all with
// the size and format of face 0; texture coordinates generated as reflection vectors.
bool OpenGLTextureCube::Init(const Texture::CreateArgsCube& args)
{
    LDD_GL_TRY
    const OpenGLExtensions& extensions = m_render->GetExtensions();
    if (!extensions.m_textureCubeMap)
        return false;
    m_target = GL_TEXTURE_CUBE_MAP_ARB;
    GLPixelFormat pf;
    if (!GetImageFormat(args.faces[0].m_format, args.alphaMode, args.mipmapMode,
                        extensions.m_textureCompression, pf, m_format))
        return false;
    CreateGLTexture(args.priority, args.wrapMode, args.mipmapMode, Texture::kFilterLinear);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    SetSizes(args.faces[0].m_width, args.faces[0].m_height);
    for (UInt32 face = 0; face < 6; ++face) {
        if (m_target != GL_TEXTURE_CUBE_MAP_ARB)
            return false;
        if (!UploadImage(args.faces[face], (UInt32)m_imageSize.x, (UInt32)m_imageSize.y,
                         GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + face, args.mipmapMode,
                         pf.bytesPerPixel, pf.internalFormat, pf.format, pf.type))
            return false;
    }
    glBindTexture(m_target, 0);
    NoteGLStateChanged();
    return true;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// ===========================================================================================
// OpenGLTexture2DRenderTargetFBO
// ===========================================================================================

// Mac 0x226cde area (inline), Win 0x4f5dd0
OpenGLTexture2DRenderTargetFBO::OpenGLTexture2DRenderTargetFBO(OpenGLRender* render)
    : OpenGLTexture2D(render), m_framebuffer(0), m_depthBuffer()
{
}

// Mac 0xf9790, Win 0x502370
bool OpenGLTexture2DRenderTargetFBO::Init(const Texture::RenderTargetCreateArgs2D& args,
                                          OpenGLTextureFactory* factory)
{
    LDD_GL_TRY
    if (!m_render->GetExtensions().m_framebufferObject)
        return false;
    if (m_render->GetRenderTarget() != 0)
        m_render->SetRenderTarget(0);
    GLPixelFormat pf;
    if (!GetRenderTargetFormat(args.format, args.alphaMode, pf, m_format))
        return false;
    m_target = GL_TEXTURE_2D;
    SetSizes(args.width, args.height);
    glGenFramebuffersEXT(1, (GLuint*)&m_framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_framebuffer);
    GenerateTexture(m_textureID, m_target, 1.0f);
    GLint wrap = WrapParameter(args.wrapMode);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ClearGLErrors();
    glTexImage2D(m_target, 0, pf.internalFormat, (GLsizei)m_imageSize.x, (GLsizei)m_imageSize.y, 0,
                 pf.format, pf.type, 0);
    if (!CheckGLError())
        return false;
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, m_target, m_textureID, 0);
    if (args.depthBuffer) {
        m_depthBuffer = factory->GetDepthBuffer(m_imageSize);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D,
                                  m_depthBuffer->GetTextureName(0), 0);
    }
    // NOTE: on failure the framebuffer stays bound.
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
        return false;
    glBindTexture(m_target, 0);
    NoteGLStateChanged();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    return true;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xf9d40
void OpenGLTexture2DRenderTargetFBO::SetDepthBuffer(Texture* depthBuffer)
{
    if (depthBuffer == 0)
        return;
    OpenGLTexture2DDepthBuffer* depth = dynamic_cast<OpenGLTexture2DDepthBuffer*>(depthBuffer);
    if (depth == 0)
        return;
    m_depthBuffer = depth;
    Texture* current = m_render->GetRenderTarget();
    if (current == this) {
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D,
                                  m_depthBuffer->GetTextureName(0), 0);
        glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    } else {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_framebuffer);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D,
                                  m_depthBuffer->GetTextureName(0), 0);
        glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        // NOTE: reads the current target's +0x60 as its framebuffer: right for this class, the
        // first colour texture name for an MRT target.
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,
                             current ? static_cast<OpenGLTexture2DRenderTargetFBO*>(current)->m_framebuffer : 0);
    }
}

// Mac 0xfc9d0, Win 0x4f6130
void OpenGLTexture2DRenderTargetFBO::NotifyChanged(UInt32 face)
{
    NotifyObservers(face);
}

// Mac 0xf9e80, Win 0x502680
void OpenGLTexture2DRenderTargetFBO::ReleaseGLObject()
{
    if (m_framebuffer != 0) {
        if (m_render->GetRenderTarget() != 0)
            m_render->SetRenderTarget(0);
        glDeleteFramebuffersEXT(1, (const GLuint*)&m_framebuffer);
        m_framebuffer = 0;
    }
    OpenGLTexture::ReleaseGLObject();
}

// ===========================================================================================
// OpenGLTexture2DMultipleRenderTargetFBO
// ===========================================================================================

// Win 0x4f5f30
OpenGLTexture2DMultipleRenderTargetFBO::OpenGLTexture2DMultipleRenderTargetFBO(OpenGLRender* render)
    : OpenGLTexture2D(render), m_framebuffer(0), m_depthBuffer()
{
    for (UInt32 i = 0; i < 4; ++i)
        m_textures[i] = 0;
}

// Mac 0xf9f40, Win 0x5026f0
bool OpenGLTexture2DMultipleRenderTargetFBO::Init(const Texture::MultipleRenderTargetCreateArgs2D& args,
                                                  OpenGLTextureFactory* factory)
{
    LDD_GL_TRY
    if (!m_render->GetExtensions().m_framebufferObject)
        return false;
    if (m_render->GetRenderTarget() != 0)
        m_render->SetRenderTarget(0);
    GLPixelFormat pf;
    if (!GetRenderTargetFormat(args.format, args.alphaMode, pf, m_format))
        return false;
    m_target = GL_TEXTURE_2D;
    SetSizes(args.width, args.height);
    glGenFramebuffersEXT(1, (GLuint*)&m_framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_framebuffer);
    for (UInt32 i = 0; i < args.count; ++i) {
        GenerateTexture(m_textures[i], m_target, 1.0f);
        GLint wrap = WrapParameter(args.wrapMode);
        glTexParameteri(m_target, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(m_target, GL_TEXTURE_WRAP_T, wrap);
        glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ClearGLErrors();
        glTexImage2D(m_target, 0, pf.internalFormat, (GLsizei)m_imageSize.x, (GLsizei)m_imageSize.y,
                     0, pf.format, pf.type, 0);
        if (!CheckGLError())
            return false;
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + i, m_target,
                                  m_textures[i], 0);
    }
    m_textureID = m_textures[0];
    if (args.depthBuffer) {
        m_depthBuffer = factory->GetDepthBuffer(m_imageSize);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D,
                                  m_depthBuffer->GetTextureName(0), 0);
    }
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
        return false;
    glBindTexture(m_target, 0);
    NoteGLStateChanged();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    return true;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xfa590, Win 0x5029b0
UInt32 OpenGLTexture2DMultipleRenderTargetFBO::GetTextureName(UInt32 attachment) const
{
    return m_textures[attachment];
}

// Mac 0xfcd90, Win 0x4f6130
void OpenGLTexture2DMultipleRenderTargetFBO::NotifyChanged(UInt32 face)
{
    NotifyObservers(face);
}

// Mac 0xfa510, Win 0x502940.
// NOTE: only the first colour texture (m_textureID) is deleted; the others leak. Fixed (low).
void OpenGLTexture2DMultipleRenderTargetFBO::ReleaseGLObject()
{
    if (m_framebuffer != 0) {
        if (m_render->GetRenderTarget() != 0)
            m_render->SetRenderTarget(0);
        glDeleteFramebuffersEXT(1, (const GLuint*)&m_framebuffer);
        m_framebuffer = 0;
    }
    for (UInt32 i = 1; i < 4; ++i) {
        if (m_textures[i] != 0 && m_textures[i] != m_textureID)
            glDeleteTextures(1, (const GLuint*)&m_textures[i]);
        NoteGLStateChanged();
        m_textures[i] = 0;
    }
    m_textures[0] = 0;
    OpenGLTexture::ReleaseGLObject();
}

// ===========================================================================================
// OpenGLTexture2DDepthBuffer
// ===========================================================================================

// Mac 0xfa5a0, Win 0x5029c0
bool OpenGLTexture2DDepthBuffer::Init(const Texture::DepthBufferCreateArgs2D& args)
{
    LDD_GL_TRY
    if (!m_render->GetExtensions().m_framebufferObject)
        return false;
    UInt32 internalFormat = GL_DEPTH_COMPONENT16 + args.depthFormat;
    m_target = GL_TEXTURE_2D;
    SetSizes(args.width, args.height);
    GenerateTexture(m_textureID, GL_TEXTURE_2D, 1.0f);
    GLint wrap = WrapParameter(args.wrapMode);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);
    ClearGLErrors();
    glTexImage2D(m_target, 0, internalFormat, (GLsizei)m_imageSize.x, (GLsizei)m_imageSize.y, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
    if (!CheckGLError())
        return false;
    glBindTexture(m_target, 0);
    NoteGLStateChanged();
    return true;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xfd140, Win 0x4f6130
void OpenGLTexture2DDepthBuffer::NotifyChanged(UInt32 face)
{
    NotifyObservers(face);
}

// Mac 0xfa870, Win 0x502b80. Leaves the factory's depth-buffer list (keyed by GetSize()).
// NOTE: deletes the name as a renderbuffer and then as a texture; a name of 0 skips the first
// part and "deletes" texture 0.
void OpenGLTexture2DDepthBuffer::ReleaseGLObject()
{
    GLuint name;
    if (m_textureID == 0) {
        name = 0;
    } else {
        glDeleteRenderbuffersEXT(1, (const GLuint*)&m_textureID);
        if (m_owner) {
            m_owner->RemoveDepthBuffer(GetSize());
            m_owner = 0;
        }
        name = m_textureID;
        if (name == (GLuint)-1)
            return;
    }
    glDeleteTextures(1, &name);
    NoteGLStateChanged();
    m_textureID = (UInt32)-1;
}

// ===========================================================================================
// Initialisers of the mutable textures (their other methods: OpenGLTextureFactory.cpp)
// ===========================================================================================

// Mac 0xf9160, Win 0x502190
bool OpenGLTexture2DMutable::Init(const Texture::MutableCreateArgs2D& args)
{
    LDD_GL_TRY
    GLPixelFormat pf;
    bool known = GetMutableFormat(args.format, args.alphaMode, pf, m_format);
    m_glType = pf.type;
    if (!known)
        return false;
    m_glFormat = pf.format;
    m_target = GL_TEXTURE_2D;
    GenerateTexture(m_textureID, GL_TEXTURE_2D, args.priority);
    GLint wrap = WrapParameter(args.wrapMode);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_bitmap = new OpenGLBitmapObject(args.width, args.height, args.format, this, 0);
    SetSizes(args.width, args.height);
    if (!UploadImageLevel0(m_bitmap->GetBitmap(), (UInt32)m_imageSize.x, (UInt32)m_imageSize.y,
                           m_target, pf.bytesPerPixel, pf.internalFormat, m_glFormat, m_glType))
        return false;
    glBindTexture(m_target, 0);
    NoteGLStateChanged();
    return true;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xfb020, Win 0x502e10
bool OpenGLTextureCubeMutable::Init(const Texture::MutableCreateArgsCube& args)
{
    LDD_GL_TRY
    if (!m_render->GetExtensions().m_textureCubeMap)
        return false;
    m_target = GL_TEXTURE_CUBE_MAP_ARB;
    GLPixelFormat pf;
    bool known = GetMutableFormat(args.format, args.alphaMode, pf, m_format);
    m_glType = pf.type;
    if (!known)
        return false;
    m_glFormat = pf.format;
    GenerateTexture(m_textureID, GL_TEXTURE_CUBE_MAP_ARB, args.priority);
    GLint wrap = WrapParameter(args.wrapMode);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    SetSizes(args.width, args.height);
    for (UInt32 face = 0; face < 6; ++face) {
        if (m_target != GL_TEXTURE_CUBE_MAP_ARB)
            return false;
        m_bitmaps[face] = new OpenGLBitmapObject(args.width, args.height, args.format, this, face);
        if (!UploadImageLevel0(m_bitmaps[face]->GetBitmap(), (UInt32)m_imageSize.x,
                               (UInt32)m_imageSize.y, GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + face,
                               pf.bytesPerPixel, pf.internalFormat, m_glFormat, m_glType))
            return false;
    }
    glBindTexture(m_target, 0);
    NoteGLStateChanged();
    return true;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// ===========================================================================================
// OpenGLBitmapObject
// ===========================================================================================

// Mac 0xf96b0
OpenGLBitmapObject::OpenGLBitmapObject(UInt32 width, UInt32 height, UInt32 format,
                                       OpenGLTexture* texture, UInt32 face)
    : m_texture(texture), m_face(face)
{
    UInt32 alignedWidth = (width + 3) & ~3u;
    UInt32 bitsPerPixel = format & 0xff;
    UInt32 pitch = bitsPerPixel < 8 ? (bitsPerPixel * alignedWidth + 7) >> 3
                                    : (bitsPerPixel >> 3) * alignedWidth;
    m_bitmap.m_width = width;
    m_bitmap.m_height = height;
    m_bitmap.m_depth = 1;
    m_bitmap.m_format = format;
    m_bitmap.m_dataSize = pitch * height;
    m_bitmap.m_pitch = (Int32)pitch;
    m_bitmap.m_data = new UInt8[pitch * height];
    m_bitmap.m_palette = 0;
}

// Mac 0xfda00/0xfda40, Win 0x5018a0
OpenGLBitmapObject::~OpenGLBitmapObject()
{
    if (m_bitmap.m_data)
        delete[] static_cast<UInt8*>(m_bitmap.m_data);
}

// Mac 0xfd980, Win 0x501700
void OpenGLBitmapObject::Update()
{
    LDD_GL_TRY
    if (m_texture)
        m_texture->NotifyChanged(m_face);
    LDD_GL_CATCH
}

// (M25, see OpenGLTexture.h; at the end of the file so that no LDD_GL_CATCH line moves)
OpenGLUnpackAlignment1::OpenGLUnpackAlignment1()
    : m_saved(4)
{
    GLint saved = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &saved);
    m_saved = (Int32)saved;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

OpenGLUnpackAlignment1::~OpenGLUnpackAlignment1()
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint)m_saved);
}

} // namespace LEGO
