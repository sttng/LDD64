// LEGO Digital Designer — restored source
// LEGO::OpenGLTextureFactory, OpenGLTexture2DMutable / OpenGLTextureCube / OpenGLTextureCubeMutable
// methods — see OpenGLTextureFactory.h / OpenGLTexture.h. TU 168 (Mac 0x226870-0x2290ff).
#include "OpenGLTextureFactory.h"
#include "OpenGLTexture.h"
#include "OpenGLRender.h"
#include "OpenGLFactories.h"
#include "File.h"
#include "Trace.h"
#include "DataStream.h"
#if defined(_WIN32)
#include "BitmapCodec.h"
#else
#include "MacImageIO.h"
#endif

#include <math.h>
#include <typeinfo>

namespace LEGO {

namespace {

#if defined(_WIN32)
// Win 0x4f5180: bytes per row of `width` pixels of `bitsPerPixel`.
UInt32 RowBytes(UInt32 width, UInt32 bitsPerPixel)
{
    if (bitsPerPixel >= 8)
        return (bitsPerPixel >> 3) * width;
    return (width * bitsPerPixel + 7) >> 3;
}

// Win 0x4f52c0: alpha-first byte orders → alpha-last ones (through SwapRedBlueFormat 0x4f5310).
UInt32 AlphaLastFormat(UInt32 format)
{
    switch (Bitmap::SwapRedBlueFormat(format)) {
    case kBitmapFormatBGRA32: return kBitmapFormatRGBA32;
    case kBitmapFormatBGRX32: return kBitmapFormatRGBX32;
    case kBitmapFormatRGBX32: return kBitmapFormatBGRX32;
    case kBitmapFormatRGBA32: return kBitmapFormatBGRA32;
    default:                  return format;
    }
}

// Win 0x4f51b0: the format with an alpha channel the decoder converts a colour-keyed image to.
// (name inferred)
UInt32 AddAlphaFormat(UInt32 format)
{
    switch (format) {
    case kBitmapFormatRGB565:   return AlphaLastFormat(kBitmapFormatRGBA32);
    case kBitmapFormatBGR24:    return AlphaLastFormat(kBitmapFormatABGR32);
    case kBitmapFormatBGRX32:   return AlphaLastFormat(kBitmapFormatBGRA32);
    case kBitmapFormatXRGB1555: return AlphaLastFormat(kBitmapFormatARGB1555);
    case kBitmapFormatRGB24:    return AlphaLastFormat(kBitmapFormatRGBA32);
    case kBitmapFormatXBGR32:   return AlphaLastFormat(kBitmapFormatABGR32);
    case kBitmapFormatXRGB4444: return AlphaLastFormat(kBitmapFormatARGB4444);
    case kBitmapFormatXRGB32:   return AlphaLastFormat(kBitmapFormatARGB32);
    case kBitmapFormatRGBX32:   return AlphaLastFormat(kBitmapFormatRGBA32);
    case kBitmapFormatBGRA32:   return AlphaLastFormat(kBitmapFormatBGRA32);
    case kBitmapFormatARGB1555: return AlphaLastFormat(kBitmapFormatARGB1555);
    case kBitmapFormatARGB4444: return AlphaLastFormat(kBitmapFormatARGB4444);
    case kBitmapFormatABGR32:   return AlphaLastFormat(kBitmapFormatABGR32);
    case kBitmapFormatARGB32:   return AlphaLastFormat(kBitmapFormatARGB32);
    case kBitmapFormatRGBA32:   return AlphaLastFormat(kBitmapFormatRGBA32);
    // the indexed formats (jump table; Win 0x4f5299 is a fragment of this function)
    case kBitmapFormatIndexed1: return AlphaLastFormat(kBitmapFormatRGBA32);
    case kBitmapFormatIndexed2: return AlphaLastFormat(kBitmapFormatRGBA32);
    case kBitmapFormatIndexed4: return AlphaLastFormat(kBitmapFormatRGBA32);
    case kBitmapFormatIndexed8: return AlphaLastFormat(kBitmapFormatRGBA32);
    default:                    return AlphaLastFormat(format);
    }
}
#endif

// Mac 0x227680: the depth-buffer size, rounded to the nearest integer (ties up), then up to a
// power of two.
UInt32 RoundToPowerOfTwo(Real32 value)
{
    Real32 lower = floorf(value);
    Real32 upper = ceilf(value);
    Real32 nearest = (value - lower < upper - value) ? lower : upper;
    UInt32 integer = (UInt32)(Int64)nearest;
    UInt32 power = 1;
    UInt32 previous;
    do {
        previous = power;
        power = previous * 2;
    } while (previous < integer);
    return previous;
}

// Release of an object nobody referenced yet (refcount 0 → deleted).
template <class T>
void Discard(T* object)
{
    object->Release();
}

} // namespace

// Mac 0x226870, Win 0x4f6ff0
Object* OpenGLTextureFactory::CreateObject(const ObjectFactoryArgs& args)
{
    if (const Texture::LoadArgs2D* load = dynamic_cast<const Texture::LoadArgs2D*>(&args))
        return LoadTexture(*load);

    if (const Texture::CreateArgs2D* create = dynamic_cast<const Texture::CreateArgs2D*>(&args)) {
        m_render->MakeCurrent();
        OpenGLTexture2D* texture = new OpenGLTexture2D(m_render);
        if (!texture->Init(*create)) {
            Discard(texture);
            return 0;
        }
        return texture;
    }

    if (const Texture::MutableCreateArgs2D* create = dynamic_cast<const Texture::MutableCreateArgs2D*>(&args)) {
        m_render->MakeCurrent();
        OpenGLTexture2DMutable* texture = new OpenGLTexture2DMutable(m_render);
        if (!texture->Init(*create)) {
            Discard(texture);
            return 0;
        }
        return texture;
    }

    const OpenGLExtensions& extensions = m_render->GetExtensions();
    if (extensions.m_framebufferObject) {
        if (const Texture::RenderTargetCreateArgs2D* create =
                dynamic_cast<const Texture::RenderTargetCreateArgs2D*>(&args)) {
            m_render->MakeCurrent();
            OpenGLTexture2DRenderTargetFBO* texture = new OpenGLTexture2DRenderTargetFBO(m_render);
            if (!texture->Init(*create, this)) {
                Discard(texture);
                return 0;
            }
            return texture;
        }
        if (const Texture::DepthBufferCreateArgs2D* create =
                dynamic_cast<const Texture::DepthBufferCreateArgs2D*>(&args)) {
            m_render->MakeCurrent();
            OpenGLTexture2DDepthBuffer* texture = new OpenGLTexture2DDepthBuffer(m_render);
            if (!texture->Init(*create)) {
                Discard(texture);
                return 0;
            }
            return texture;
        }
    }
    if (extensions.m_drawBuffers) {
        if (const Texture::MultipleRenderTargetCreateArgs2D* create =
                dynamic_cast<const Texture::MultipleRenderTargetCreateArgs2D*>(&args)) {
            m_render->MakeCurrent();
            OpenGLTexture2DMultipleRenderTargetFBO* texture =
                new OpenGLTexture2DMultipleRenderTargetFBO(m_render);
            if (!texture->Init(*create, this)) {
                Discard(texture);
                return 0;
            }
            return texture;
        }
    }
    if (const Texture::CreateArgsCube* create = dynamic_cast<const Texture::CreateArgsCube*>(&args)) {
        m_render->MakeCurrent();
        OpenGLTextureCube* texture = new OpenGLTextureCube(m_render);
        if (!texture->Init(*create)) {
            Discard(texture);
            return 0;
        }
        return texture;
    }
    if (const Texture::MutableCreateArgsCube* create = dynamic_cast<const Texture::MutableCreateArgsCube*>(&args)) {
        m_render->MakeCurrent();
        OpenGLTextureCubeMutable* texture = new OpenGLTextureCubeMutable(m_render);
        if (!texture->Init(*create)) {
            Discard(texture);
            return 0;
        }
        return texture;
    }
    if (const Texture::CreateArgs3D* create = dynamic_cast<const Texture::CreateArgs3D*>(&args)) {
        m_render->MakeCurrent();
        OpenGLTexture3D* texture = new OpenGLTexture3D(m_render);
        if (!texture->Init(*create)) {
            Discard(texture);
            return 0;
        }
        return texture;
    }
    return 0;
}

// Mac 0x2268b5-0x226eca, Win 0x4f7026-0x4f71c1
Object* OpenGLTextureFactory::LoadTexture(const Texture::LoadArgs2D& load)
{
    BasicMap<Path, OpenGLTexture2D*>::Iterator cached = m_textures.Find(load.path);
    if (cached != m_textures.End())
        return cached->second;   // NOTE: a null entry would be returned as null (both builds check)

    Texture::CreateArgs2D args(load.create);
    m_render->MakeCurrent();
    Object* result = 0;
    if (DecodeImageFile(load, args)) {
        OpenGLTexture2D* texture = new OpenGLTexture2D(m_render);
        if (!texture->Init(args)) {
            Discard(texture);
        } else {
            m_textures[load.path] = texture;
            texture->SetSource(load.path, this);
            result = texture;
        }
    }
    if (args.bitmap.m_data)
        delete[] static_cast<UInt8*>(args.bitmap.m_data);
    return result;
}

#if defined(_WIN32)

// Win 0x4f6c20 (file → codec) + 0x4f6d90 (header, size check, alpha mode, decode).
bool OpenGLTextureFactory::DecodeImageFile(const Texture::LoadArgs2D& load, Texture::CreateArgs2D& args)
{
    const Color* colorKey = load.useColorKey ? &load.colorKey : 0;
    // 0x4f6c20
    RefPointer<BitmapCodec> codec;
    {
        RefPointer<File> file = load.path.OpenFile(OpenMode(true, false, true, false), false);
        if (file.IsNull())
            return false;
        UInt32 size = (UInt32)file->GetSize();
        UInt8* buffer = new UInt8[size];
        RefPointer<DataStream> stream = new DataStream(buffer, (UInt32)file->GetSize(), true);
        if (!file->Read(stream->GetBufferSize(), stream->GetBuffer(), 0))
            return false;
        BitmapCodec::LoadArgs codecArgs(stream);
        codec = dynamic_cast<BitmapCodec*>(ObjectFactory::GetInstance()->CreateObject(typeid(BitmapCodec), codecArgs));
    }
    if (codec.IsNull())
        return false;
    // 0x4f6d90
    RefPointer<BitmapCodec::Header> header = codec->ReadHeader();
    if (header.IsNull())
        return false;
    const BitmapCodec::Surface& surface = header->m_surfaces[0][0];
    Bitmap& bitmap = args.bitmap;
    bitmap.m_width = surface.m_width;
    bitmap.m_height = surface.m_height;
    if (bitmap.m_width > 0x400 || bitmap.m_height > 0x400) {
        // NOTE: the original converts the path to a native one (Platform 0x412db0); the VFS
        // path is traced here.
        String text("Images should not be larger than 1024 pixels: ");
        text << load.path.GetPath(true);
        Trace(text, true);
        return false;
    }
    bitmap.m_format = surface.m_format;
    if (bitmap.m_format & kBitmapFormatAlphaFlag) {
        args.alphaMode = Texture::kAlphaImage;
    } else if (colorKey) {
        args.alphaMode = Texture::kAlphaColorKey;
        bitmap.m_format = AddAlphaFormat(bitmap.m_format);
    } else {
        args.alphaMode = Texture::kAlphaNone;
    }
    UInt32 pitch = RowBytes(bitmap.m_width, bitmap.m_format & 0xff);
    bitmap.m_pitch = (Int32)pitch;
    bitmap.m_dataSize = bitmap.m_height * pitch;
    bitmap.m_data = new UInt8[bitmap.m_height * pitch];
    return codec->Decode(bitmap, colorKey);
}

#else

// Mac 0x2269e8-0x226e22 (ImageIO block: MacImageIO.h DecodeImageWithImageIO).
bool OpenGLTextureFactory::DecodeImageFile(const Texture::LoadArgs2D& load, Texture::CreateArgs2D& args)
{
    RefPointer<File> file = load.path.OpenFile(OpenMode(true, false, true, false), false);
    if (file.IsNull())
        return false;
    UInt32 size = (UInt32)file->GetSize();
    UInt8* buffer = new UInt8[size];
    RefPointer<DataStream> stream = new DataStream(buffer, (UInt32)file->GetSize(), true);
    if (!file->Read(stream->GetBufferSize(), stream->GetBuffer(), 0))
        return false;
    Bitmap decoded;
    if (!DecodeImageWithImageIO(stream->GetBuffer(), stream->GetBufferSize(), decoded)) {
        // NOTE: the original traces Platform::VFSToNativePath(path) (Mac 0xca90); the VFS path
        // is traced here.
        String text("Images should not be larger than 1024 pixels: ");
        text << load.path.GetPath(true);
        Trace(text, true);
        return false;
    }
    // only these fields are taken over (depth and palette keep the arguments' values)
    args.bitmap.m_width = decoded.m_width;
    args.bitmap.m_height = decoded.m_height;
    args.bitmap.m_pitch = decoded.m_pitch;
    args.bitmap.m_dataSize = decoded.m_dataSize;
    args.bitmap.m_data = decoded.m_data;
    args.bitmap.m_format = kBitmapFormatBGRA32;
    args.alphaMode = Texture::kAlphaImage;
    return true;
}

#endif

// Mac 0x227680
OpenGLTexture2DDepthBuffer* OpenGLTextureFactory::GetDepthBuffer(const Vector2& size)
{
    MutexLocker lock(m_mutex.Get());
    for (BasicArray<DepthBufferEntry>::Iterator it = m_depthBuffers.Begin(); it != m_depthBuffers.End(); ++it) {
        if (it->first == size)
            return it->second;
    }
    Texture::DepthBufferCreateArgs2D args;
    args.wrapMode = Texture::kWrapClampToEdge;
    args.field08 = 0;
    args.width = RoundToPowerOfTwo(size.x);
    args.height = RoundToPowerOfTwo(size.y);
    args.depthFormat = 2;
    Object* object = CreateObject(args);
    // NOTE: not checked for null (a failed creation crashes in both builds)
    OpenGLTexture2DDepthBuffer* depth = object ? dynamic_cast<OpenGLTexture2DDepthBuffer*>(object) : 0;
    depth->SetOwner(this);
    m_depthBuffers.PushBack(DepthBufferEntry(size, depth));
    return depth;
}

// Mac 0x2275c0: removes the first entry of `size` (the rest moves up).
void OpenGLTextureFactory::RemoveDepthBuffer(const Vector2& size)
{
    MutexLocker lock(m_mutex.Get());
    for (UInt32 i = 0; i < m_depthBuffers.GetSize(); ++i) {
        if (m_depthBuffers[i].first == size) {
            m_depthBuffers.Erase(i);
            return;
        }
    }
}

// Mac 0xf8fa0 body: map erase by key (0xfd780) under the factory mutex.
void OpenGLTextureFactory::RemoveTexture(const Path& path)
{
    MutexLocker lock(m_mutex.Get());
    m_textures.Erase(path);
}

// OpenGLFactories.h seam: `new OpenGLTextureFactory(render)` in OpenGLRender::Initialize
// (Mac 0xf2fc6).
SpecificObjectFactory* CreateOpenGLTextureFactory(OpenGLRender* render)
{
    return new OpenGLTextureFactory(render);
}

// ===========================================================================================
// OpenGLTexture2DMutable (Init in OpenGLTexture.cpp)
// ===========================================================================================

// Mac 0x227c10, Win 0x4f5d10 (releases the bitmap)
OpenGLTexture2DMutable::~OpenGLTexture2DMutable()
{
}

bool OpenGLTexture2DMutable::IsMutable() const { return true; }       // Mac 0x227cf0, Win 0x4f5b70

// Mac 0x227d00, Win 0x4f5b80
RefPointer<BitmapObject> OpenGLTexture2DMutable::GetBitmap(UInt32) const
{
    return RefPointer<BitmapObject>(m_bitmap.Get());
}

// Mac 0x227d40, Win 0x4f5c00
void OpenGLTexture2DMutable::NotifyChanged(UInt32 face)
{
    m_render->UploadTextureImage(m_target, m_target, m_textureID, 0, m_glFormat, m_glType,
                                 m_bitmap->GetBitmap());
    NotifyObservers(face);
}

// Mac 0x228000, Win 0x4f5cd0: the bitmap stops forwarding Update.
void OpenGLTexture2DMutable::ReleaseGLObject()
{
    if (m_bitmap)
        m_bitmap->DetachTexture();
    OpenGLTexture::ReleaseGLObject();
}

// ===========================================================================================
// OpenGLTextureCube (Init in OpenGLTexture.cpp)
// ===========================================================================================

UInt32 OpenGLTextureCube::GetType() const { return kTypeCube; }       // Mac 0x228170, Win 0x4f6330
bool   OpenGLTextureCube::IsMutable() const { return false; }         // Mac 0x228180, Win 0x4aede0
RefPointer<BitmapObject> OpenGLTextureCube::GetBitmap(UInt32) const   // Mac 0x228190, Win 0x4f5980
{
    return RefPointer<BitmapObject>();
}

// ===========================================================================================
// OpenGLTextureCubeMutable (Init in OpenGLTexture.cpp)
// ===========================================================================================

// Mac 0x228280 (D2 0x228800: releases the six bitmaps, last first)
OpenGLTextureCubeMutable::~OpenGLTextureCubeMutable()
{
    for (Int32 face = 5; face >= 0; --face)
        m_bitmaps[face] = 0;
}

bool OpenGLTextureCubeMutable::IsMutable() const { return true; }     // Mac 0x228350, Win 0x4f5b70

// Mac 0x228360, Win 0x4f64c0
RefPointer<BitmapObject> OpenGLTextureCubeMutable::GetBitmap(UInt32 face) const
{
    return RefPointer<BitmapObject>(m_bitmaps[face].Get());
}

// Mac 0x2283a0, Win 0x4f6540
void OpenGLTextureCubeMutable::NotifyChanged(UInt32 face)
{
    m_render->UploadTextureImage(m_target, GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + face, m_textureID, 0,
                                 m_glFormat, m_glType, m_bitmaps[face]->GetBitmap());
    NotifyObservers(face);
}

// Mac 0x228690, Win 0x4f6620
void OpenGLTextureCubeMutable::ReleaseGLObject()
{
    for (UInt32 face = 0; face < 6; ++face) {
        if (m_bitmaps[face])
            m_bitmaps[face]->DetachTexture();
    }
    OpenGLTexture::ReleaseGLObject();
}

} // namespace LEGO
