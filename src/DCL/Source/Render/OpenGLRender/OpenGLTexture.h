// LEGO Digital Designer — restored source
// OpenGL textures: OpenGLTexture (abstract), 2D, 3D, Cube, 2DRenderTargetFBO,
// 2DMultipleRenderTargetFBO, 2DDepthBuffer, OpenGLBitmapObject, and the mutable 2D / Cube classes
// (methods in OpenGLTextureFactory.cpp, TU 168). Names inferred unless noted.
// TU 036 (__FILE__ Mac 0xf8ba0/0xfa8f0, "OpenGLTexture.h" 0xf9020/0xfd980): Mac 0xf87f0-0xfdeaf,
// Win 0x4f5490-0x4f8e80 and 0x5014b0-0x502b80. Slots Mac/Win:
// 0/1 D1/D0 and 2 OnFinalRelease (Win: Object vtable), 3/0 GetType, 4/1 IsMutable, 5/2 GetFormat,
// 6/3 GetBitmap, 7/4 GetSize, 8/5 GetImageSize, 9/6 NotifyChanged, 10/7 GetTextureName,
// 11/8 ReleaseGLObject. Offsets below are Mac; Win has a vbptr at +4 and another order.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Path.h"
#include "Geometry.h"
#include "Texture.h"

namespace LEGO {

class OpenGLRender;
class OpenGLTextureFactory;

// NOTE: the original never sets GL_UNPACK_ALIGNMENT, so image data is read with OpenGL's default
// of 4, but the rows of a bitmap (RowBytes pitch: Win RGB24 / BGR24 / grey images) and of the
// padding copy in UploadImage are tightly packed: an image whose (power-of-two) width is 1 or 2
// with 1 or 3 bytes per pixel is read with a skew from the second row on (and past the data);
// fixed: UploadImage and UploadImage3D upload with alignment 1 and restore the previous value
// (M25).
class OpenGLUnpackAlignment1 {   // GL_UNPACK_ALIGNMENT 1 for its scope (M25)
public:
    OpenGLUnpackAlignment1();
    ~OpenGLUnpackAlignment1();
private:
    OpenGLUnpackAlignment1(const OpenGLUnpackAlignment1&);
    OpenGLUnpackAlignment1& operator=(const OpenGLUnpackAlignment1&);
    Int32 m_saved;
};

// ===========================================================================================
// OpenGLTexture — Mac sizeof 0x48 as a base (Object at the end). Created only through the
// subclasses (ctor inlined: Mac 0x227a00 base-object ctor, Win 0x4f5490 area).
// ===========================================================================================
class OpenGLTexture : public Texture {
public:
    // Registers with the render (OpenGLRender::RegisterTexture Mac 0xf6780) and keeps it alive.
    explicit OpenGLTexture(OpenGLRender* render);                     // Mac 0x227a00
    // Unregisters (Mac 0xf69c0), releases the render.
    virtual ~OpenGLTexture();                                         // Mac 0xfdb60 (D2), 0xfbdd0/0xfbe30

    virtual UInt32  GetFormat() const;                                // Mac 0xfbea0, Win 0x5005f0
    virtual Vector2 GetSize() const;                                  // Mac 0xfbeb0, Win 0x4f5600
    virtual Vector2 GetImageSize() const;                             // Mac 0xfbee0, Win 0x4f5620

    // Contents of face/attachment `face` changed (see TextureObserver::OnTextureChanged); the
    // base does nothing.
    virtual void   NotifyChanged(UInt32 face);                        // 9  Mac 0xfbf10, Win 0x40be80
    // GL texture name of colour attachment `attachment` (only MRT targets have more than one).
    virtual UInt32 GetTextureName(UInt32 attachment) const;           // 10 Mac 0xfbf20, Win 0x4f5640
    // Deletes the GL objects (on final release and when the GL context goes away).
    virtual void   ReleaseGLObject();                                 // 11 Mac 0xf9f00, Win 0x4f5650

    UInt32        GetTarget() const { return m_target; }
    OpenGLRender* GetRender() const { return m_render; }

protected:
    // Makes the context current, deletes the GL objects, detaches the observers.
    virtual void OnFinalRelease();                                    // Mac 0xf9020 (thunk 0xfc010)

    // Notifies every observer (OnTextureChanged(face)), the NotifierT iteration the RT/MRT/depth
    // and mutable NotifyChanged overrides inline (Mac 0xfcb50 loop).
    void NotifyObservers(UInt32 face);

    // glGenTextures + glPrioritizeTextures + bind + wrap/filter parameters (Mac 0xf87f0).
    void CreateGLTexture(Real32 priority, UInt32 wrapMode, UInt32 mipmapMode, UInt32 filter);

    // Uploads `bitmap` to `imageTarget` as a widthxheight (power of two) image: directly when the
    // sizes match, else through a temporary copy (rows of bitmap.m_pitch bytes at the top-left;
    // NOTE: the rest of the copy is uninitialised heap memory). Mipmap modes 1/2 use
    // gluBuild2DMipmaps. Mac 0xf88e0, Win 0x501d20.
    static bool UploadImage(const Bitmap& bitmap, UInt32 width, UInt32 height, UInt32 imageTarget,
                            UInt32 mipmapMode, UInt32 bytesPerPixel, UInt32 internalFormat,
                            UInt32 format, UInt32 type);
    // Same without mipmaps (Mac 0xf8ab0, Win 0x5015c0?).
    static bool UploadImageLevel0(const Bitmap& bitmap, UInt32 width, UInt32 height,
                                  UInt32 imageTarget, UInt32 bytesPerPixel, UInt32 internalFormat,
                                  UInt32 format, UInt32 type);
    // Drains glGetError; false if anything but GL_NO_ERROR/GL_INVALID_OPERATION was pending
    // (Mac 0xfd680, Win 0x5014b0; the message it builds is discarded).
    static bool CheckGLError();
    // Clears pending errors (stops at GL_INVALID_OPERATION). Inlined before every glTexImage.
    static void ClearGLErrors();
    // Smallest power of two >= value (1 for 0). Inlined everywhere.
    static UInt32 NextPowerOfTwo(UInt32 value);
    // m_size = (w, h), m_imageSize = power-of-two sizes.
    void SetSizes(UInt32 width, UInt32 height);

    UInt32                   m_textureID;   // Mac +0x24 GL texture name (-1 = none)
    UInt32                   m_target;      // Mac +0x28 GL_TEXTURE_2D / _3D / _CUBE_MAP
    UInt32                   m_format;      // Mac +0x2c alphaMode of the creation arguments
    Vector2                  m_size;        // Mac +0x30 requested / source image size
    Vector2                  m_imageSize;   // Mac +0x38 GL texture size (powers of two)
    RefPointer<OpenGLRender> m_render;      // Mac +0x40

private:
    OpenGLTexture(const OpenGLTexture&);
    OpenGLTexture& operator=(const OpenGLTexture&);
};

// ===========================================================================================
// OpenGLTexture2D — Mac sizeof 0x68 (Object +0x60), Win 0x64; ctor Mac 0x227400, Win 0x4f58a0.
// Image-file textures remember their path and factory (the factory's path cache).
// ===========================================================================================
class OpenGLTexture2D : public OpenGLTexture {
public:
    explicit OpenGLTexture2D(OpenGLRender* render);
    virtual ~OpenGLTexture2D() {}                                     // Mac 0xfc710/0xfc770, Win 0x4f5a10

    virtual UInt32 GetType() const;                                   // Mac 0xfc7e0, Win 0x51e780 (1)
    virtual bool   IsMutable() const;                                 // Mac 0xfc7f0, Win 0x4aede0
    virtual RefPointer<BitmapObject> GetBitmap(UInt32 face) const;    // Mac 0xfc800, Win 0x4f5980

    // Mac 0xf8ba0, Win 0x501fc0.
    bool Init(const Texture::CreateArgs2D& args);

    // Cache entry of the factory (set by OpenGLTextureFactory::CreateObject for LoadArgs2D).
    void SetSource(const Path& path, OpenGLTextureFactory* factory) { m_path = path; m_factory = factory; }
    const Path& GetPath() const { return m_path; }

protected:
    // Leaves the factory's path cache, then OpenGLTexture::OnFinalRelease.
    virtual void OnFinalRelease();                                    // Mac 0xf8fa0 (thunk 0xf90e0)

    Path                  m_path;      // Mac +0x44 (0x18 bytes)
    OpenGLTextureFactory* m_factory;   // Mac +0x5c
};

// ===========================================================================================
// OpenGLTexture3D — Mac sizeof 0x70 (Object +0x68); ctor Mac 0x2274e0, Win 0x4f6760.
// Same path/factory members as OpenGLTexture2D (own copy of its OnFinalRelease, Mac 0xfbcd0).
// ===========================================================================================
class OpenGLTexture3D : public OpenGLTexture {
public:
    explicit OpenGLTexture3D(OpenGLRender* render);
    virtual ~OpenGLTexture3D() {}                                     // Mac 0xfd4a0/0xfd500

    virtual UInt32 GetType() const;                                   // Mac 0xfd570, Win 0x51e7d0 (2)
    virtual bool   IsMutable() const;                                 // Mac 0xfd580
    virtual RefPointer<BitmapObject> GetBitmap(UInt32 face) const;    // Mac 0xfd590

    // Mac 0xfb620, Win 0x503070.
    bool Init(const Texture::CreateArgs3D& args);

    Real32 GetDepth() const { return m_depth; }
    Real32 GetImageDepth() const { return m_imageDepth; }

protected:
    virtual void OnFinalRelease();                                    // Mac 0xfbcd0 (thunk 0xfbd50)

    Path                  m_path;        // Mac +0x44
    OpenGLTextureFactory* m_factory;     // Mac +0x5c
    Real32                m_depth;       // Mac +0x60
    Real32                m_imageDepth;  // Mac +0x64 (power of two)
};

// ===========================================================================================
// OpenGLTextureCube — Mac sizeof 0x4c (Object +0x44), Win 0x54; ctor inlined (Mac 0x227160),
// Win 0x4f6290. Methods Mac 0x228170.. (TU 168).
// ===========================================================================================
class OpenGLTextureCube : public OpenGLTexture {
public:
    explicit OpenGLTextureCube(OpenGLRender* render) : OpenGLTexture(render) {}
    virtual ~OpenGLTextureCube() {}                                   // Mac 0x2280a0/0x228100

    virtual UInt32 GetType() const;                                   // Mac 0x228170, Win 0x4f6330 (3)
    virtual bool   IsMutable() const;                                 // Mac 0x228180
    virtual RefPointer<BitmapObject> GetBitmap(UInt32 face) const;    // Mac 0x228190

    // Mac 0xfa8f0, Win 0x502bf0.
    bool Init(const Texture::CreateArgsCube& args);
};

// ===========================================================================================
// OpenGLTexture2DRenderTargetFBO — Mac sizeof 0x70 (Object +0x68), Win 0x6c; ctor Mac 0x226cde
// area, Win 0x4f5dd0.
// ===========================================================================================
class OpenGLTexture2DRenderTargetFBO : public OpenGLTexture2D {
public:
    explicit OpenGLTexture2DRenderTargetFBO(OpenGLRender* render);
    virtual ~OpenGLTexture2DRenderTargetFBO() {}                      // Mac 0xfc8f0/0xfc990, Win 0x4f5e70

    virtual void NotifyChanged(UInt32 face);                          // Mac 0xfc9d0, Win 0x4f6130
    virtual void ReleaseGLObject();                                   // Mac 0xf9e80, Win 0x502680

    // Mac 0xf9790, Win 0x502370. `factory` supplies the shared depth buffer.
    bool Init(const Texture::RenderTargetCreateArgs2D& args, OpenGLTextureFactory* factory);
    // Replaces the depth attachment (a OpenGLTexture2DDepthBuffer; anything else is ignored).
    void SetDepthBuffer(Texture* depthBuffer);                        // Mac 0xf9d40

    UInt32         GetFramebuffer() const { return m_framebuffer; }
    OpenGLTexture* GetDepthBuffer() const { return m_depthBuffer; }

    // The render's no-FBO (pbuffer) path of SetRenderTarget (Mac 0xf4b10 reads +0x60 as a
    // CGLContextObj; Win 0x4eddb0 reads +0x58 HDC, +0x5c HPBUFFERARB, +0x60 HGLRC).
    // NOTE: no code path creates a pbuffer render target (the factory refuses
    // RenderTargetCreateArgs2D without EXT_framebuffer_object, Mac 0x226cb5), so in the
    // original these reads see unrelated members; here they return the framebuffer field (Mac
    // +0x60) and null. (names inferred)
    void* GetPbufferContext() const { return reinterpret_cast<void*>((size_t)m_framebuffer); }
    void* GetPbuffer() const        { return 0; }
    void* GetPbufferDC() const      { return 0; }

protected:
    UInt32                    m_framebuffer;  // Mac +0x60 (pbuffer context without FBOs, Render)
    RefPointer<OpenGLTexture> m_depthBuffer;  // Mac +0x64
};

// ===========================================================================================
// OpenGLTexture2DMultipleRenderTargetFBO — Mac sizeof 0x80 (Object +0x78), Win 0x7c;
// ctor Win 0x4f5f30.
// ===========================================================================================
class OpenGLTexture2DMultipleRenderTargetFBO : public OpenGLTexture2D {
public:
    explicit OpenGLTexture2DMultipleRenderTargetFBO(OpenGLRender* render);
    virtual ~OpenGLTexture2DMultipleRenderTargetFBO() {}              // Mac 0xfccb0/0xfcd50, Win 0x4f5fd0

    virtual void   NotifyChanged(UInt32 face);                        // Mac 0xfcd90, Win 0x4f6130
    virtual UInt32 GetTextureName(UInt32 attachment) const;           // Mac 0xfa590, Win 0x5029b0
    virtual void   ReleaseGLObject();                                 // Mac 0xfa510, Win 0x502940

    // Mac 0xf9f40, Win 0x5026f0.
    bool Init(const Texture::MultipleRenderTargetCreateArgs2D& args, OpenGLTextureFactory* factory);

    UInt32         GetFramebuffer() const { return m_framebuffer; }
    OpenGLTexture* GetDepthBuffer() const { return m_depthBuffer; }

protected:
    UInt32                    m_textures[4];      // Mac +0x60
    UInt32                    m_framebuffer;      // Mac +0x70
    RefPointer<OpenGLTexture> m_depthBuffer;      // Mac +0x74
};

// ===========================================================================================
// OpenGLTexture2DDepthBuffer — Mac sizeof 0x6c (Object +0x64); ctor Win 0x4f6090. Shared per
// size through the factory's depth-buffer list.
// ===========================================================================================
class OpenGLTexture2DDepthBuffer : public OpenGLTexture2D {
public:
    explicit OpenGLTexture2DDepthBuffer(OpenGLRender* render)
        : OpenGLTexture2D(render), m_owner(0) {}
    virtual ~OpenGLTexture2DDepthBuffer() {}                          // Mac 0xfd070/0xfd0d0

    virtual void NotifyChanged(UInt32 face);                          // Mac 0xfd140, Win 0x4f6130
    virtual void ReleaseGLObject();                                   // Mac 0xfa870, Win 0x502b80

    // Mac 0xfa5a0, Win 0x5029c0.
    bool Init(const Texture::DepthBufferCreateArgs2D& args);

    void SetOwner(OpenGLTextureFactory* owner) { m_owner = owner; }

protected:
    OpenGLTextureFactory* m_owner;   // Mac +0x60 the factory whose list holds it
};

// ===========================================================================================
// OpenGLBitmapObject — CPU pixels of a mutable texture face. Mac sizeof 0x34 (vptr, Bitmap +4,
// texture +0x24, face +0x28, Object +0x2c); vtables Mac 0x763a8c {Update 0xfd980, D1 0xfda00,
// D0 0xfda40} / 0x763ab0; Win 0xa27ce4 {Update 0x501700} / 0xa27cec {sdd 0x5018a0}.
// ===========================================================================================
class OpenGLBitmapObject : public BitmapObject {
public:
    // Mac 0xf96b0 (allocates rows of ((width + 3) & ~3) pixels, uninitialised).
    OpenGLBitmapObject(UInt32 width, UInt32 height, UInt32 format, OpenGLTexture* texture,
                       UInt32 face);
    virtual ~OpenGLBitmapObject();                                    // Mac 0xfda00/0xfda40

    // Tells the texture that face `m_face` changed (it re-uploads the pixels).
    virtual void Update();                                            // Mac 0xfd980, Win 0x501700

    // The texture clears this when it releases its GL object (Mac 0x228000).
    void DetachTexture() { m_texture = 0; }

private:
    OpenGLTexture* m_texture;   // Mac +0x24 (not referenced)
    UInt32         m_face;      // Mac +0x28
};

// ===========================================================================================
// OpenGLTexture2DMutable — Mac sizeof 0x74 (Object +0x6c), Win 0x70; methods TU 168.
// ===========================================================================================
class OpenGLTexture2DMutable : public OpenGLTexture2D {
public:
    explicit OpenGLTexture2DMutable(OpenGLRender* render)
        : OpenGLTexture2D(render), m_glFormat(0), m_glType(0) {}
    virtual ~OpenGLTexture2DMutable();                                // Mac 0x227c10/0x227cb0, Win 0x4f5d10

    virtual bool IsMutable() const;                                   // Mac 0x227cf0, Win 0x4f5b70
    virtual RefPointer<BitmapObject> GetBitmap(UInt32 face) const;    // Mac 0x227d00, Win 0x4f5b80
    // Re-uploads the CPU bitmap (OpenGLRender::UploadTextureImage), then notifies.
    virtual void NotifyChanged(UInt32 face);                          // Mac 0x227d40, Win 0x4f5c00
    virtual void ReleaseGLObject();                                   // Mac 0x228000, Win 0x4f5cd0

    // Mac 0xf9160 (TU 036), Win 0x502190 (ctor Win 0x4f5ac0).
    bool Init(const Texture::MutableCreateArgs2D& args);

protected:
    RefPointer<OpenGLBitmapObject> m_bitmap;    // Mac +0x60
    UInt32                         m_glFormat;  // Mac +0x64
    UInt32                         m_glType;    // Mac +0x68
};

// ===========================================================================================
// OpenGLTextureCubeMutable — Mac sizeof 0x6c (Object +0x64); methods TU 168.
// ===========================================================================================
class OpenGLTextureCubeMutable : public OpenGLTextureCube {
public:
    explicit OpenGLTextureCubeMutable(OpenGLRender* render)
        : OpenGLTextureCube(render), m_glFormat(0), m_glType(0) {}
    virtual ~OpenGLTextureCubeMutable();                              // Mac 0x228280/0x2282e0 (0x228800)

    virtual bool IsMutable() const;                                   // Mac 0x228350, Win 0x4f5b70
    virtual RefPointer<BitmapObject> GetBitmap(UInt32 face) const;    // Mac 0x228360, Win 0x4f64c0
    virtual void NotifyChanged(UInt32 face);                          // Mac 0x2283a0, Win 0x4f6540
    virtual void ReleaseGLObject();                                   // Mac 0x228690, Win 0x4f6620

    // Mac 0xfb020 (TU 036), Win 0x502e10 (ctor Win 0x4f63e0).
    bool Init(const Texture::MutableCreateArgsCube& args);

protected:
    RefPointer<OpenGLBitmapObject> m_bitmaps[6];  // Mac +0x44
    UInt32                         m_glFormat;    // Mac +0x5c
    UInt32                         m_glType;      // Mac +0x60
};

} // namespace LEGO
