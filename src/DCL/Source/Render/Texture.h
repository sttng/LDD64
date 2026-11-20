// LEGO Digital Designer — restored source
// LEGO::Texture (abstract: 2D, 3D, cube, render targets), TextureObserver, BitmapObject (CPU
// pixels of a mutable texture) and the factory argument blocks (CreateArgs2D/3D/Cube, LoadArgs2D,
// Mutable*, RenderTarget*, DepthBuffer*, MultipleRenderTarget*). Header-only: D1/D0/OnFinalRelease
// in TU 036 (Mac 0xfc030/0xfc0a0/0xfc120). Created via OpenGLTextureFactory::CreateObject (Mac
// 0x226870, Win 0x4f6ff0). Vtables Mac 0x7089ac {D1, D0, OnFinalRelease 0xfc120, 6 pure}, Win
// 0xa270d0 (Mac 3-8); OpenGLTexture adds Mac 9-11 = Win 6-8. No own members (Notifier part to
// +0x24, then Object). Argument layouts from the initialisers (Mac 0xf8ba0, 0xf9160, 0xf9790,
// 0xf9f40, 0xfa5a0, 0xfa8f0, 0xfb020, 0xfb620) and users (Mac 0x1540e0, 0x35ace0, 0x227680);
// field names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Notifier.h"
#include "ObjectFactory.h"
#include "Geometry.h"
#include "Color.h"
#include "LEGO/String.h"
#include "Path.h"
#include "Bitmap.h"

namespace LEGO {

class Texture;

// ===========================================================================================
// TextureObserver — Mac vtable 0x707bec {D1 0xecb60, D0 0xecb90, OnFinalRelease 0xecbe0,
// OnAttached 0xec710, OnDetached 0xec720, pure}; Win {OnAttached, OnDetached, pure} (+ Object).
// ===========================================================================================
class TextureObserver : public Observer<Texture, TextureObserver> {
public:
    virtual ~TextureObserver() {}
    // Called by the texture when its contents changed: a mutable texture re-uploaded face `face`
    // of its CPU bitmap (OpenGLTexture2DMutable Mac 0x227d40, CubeMutable 0x2283a0), or a render
    // target was rendered into (OpenGLTexture2DRenderTargetFBO Mac 0xfc9d0, called by the
    // render with 0). The argument is passed through unchanged. (name inferred)
    virtual void OnTextureChanged(UInt32 face) = 0;                   // Mac slot 5, Win slot 2
};

// ===========================================================================================
// BitmapObject — a ref-counted CPU bitmap (RTTI name). Mac: vmi class with virtual base Object,
// no vtable of its own emitted (abstract); Win vtable 0xa27cc8 {_purecall}. One pure virtual:
// Update (OpenGLBitmapObject Mac 0xfd980, Win 0x501700) — the owner calls it after writing the
// pixels. Layout Mac {vptr, Bitmap +0x04, Object +0x2c-ish in the derived class}. (name of the
// method inferred)
// ===========================================================================================
class BitmapObject : public virtual Object {
public:
    BitmapObject() {}
    virtual void Update() = 0;                                        // Mac slot 0
    virtual ~BitmapObject() {}

    Bitmap&       GetBitmap()       { return m_bitmap; }
    const Bitmap& GetBitmap() const { return m_bitmap; }

protected:
    Bitmap m_bitmap;   // Mac +0x04, Win +0x08
};

// ===========================================================================================
// Texture
// ===========================================================================================
class Texture : public Notifier<TextureObserver> {
public:
    // GetType() values (OpenGLTexture2D 0xfc7e0 → 1, 3D 0xfd570 → 2, Cube 0x228170 → 3;
    // Render::SetTexture enables GL_TEXTURE_GEN_S/T/R for cube maps).
    enum Type { kType2D = 1, kType3D = 2, kTypeCube = 3 };

    // CreateArgs*::wrapMode → GL_TEXTURE_WRAP_S/T(/R) (Mac 0xf87f0).
    enum WrapMode { kWrapRepeat = 0, kWrapClampToEdge = 1, kWrapMirroredRepeat = 2 };
    // CreateArgs2D/3D/Cube::mipmapMode: 1 and 2 build mipmaps (gluBuild2DMipmaps, minification
    // GL_*_MIPMAP_*); 2 and 3 allow texture compression of 24/32-bit RGB(A) images when the
    // driver has ARB_texture_compression.
    enum MipmapMode { kMipmapNone = 0, kMipmap = 1, kMipmapCompressed = 2, kCompressed = 3 };
    // CreateArgs2D::filter: 1 linear, anything else nearest.
    enum Filter { kFilterNearest = 0, kFilterLinear = 1 };
    // alphaMode values the Win image loader writes (Win 0x4f6d90): 0 opaque, 1 colour-keyed
    // alpha added, 2 alpha from the image (the Mac loader always writes 2). Kept as the
    // texture's GetFormat() for formats with alpha. (names inferred)
    enum AlphaMode { kAlphaNone = 0, kAlphaColorKey = 1, kAlphaImage = 2 };

    // -------------------------------------------------------------------------------------
    // Argument blocks (all ObjectFactoryArgs; RTTI structs).
    // -------------------------------------------------------------------------------------

    // Mac vtable 0x778728 {D1 0x530f0, D0 0x53180}, Win sdd 0x455e80. Mac/Win sizeof 0x38.
    // Read by Mac 0xf8ba0 / Win 0x501fc0. Default values from the LoadArgs2D constructor
    // (Mac 0x52fc0, which embeds one).
    struct CreateArgs2D : public ObjectFactoryArgs {
        CreateArgs2D()
            : priority(0.0f), wrapMode(kWrapRepeat), alphaMode(kAlphaNone), mipmapMode(kMipmapNone),
              filter(kFilterLinear)
        {
            bitmap.m_depth = 1;
        }
        virtual ~CreateArgs2D() {}
        Real32 priority;     // +0x04 glPrioritizeTextures (Skybox sets field +0x08 to 1 — wrapMode clamp)
        UInt32 wrapMode;     // +0x08 WrapMode
        UInt32 alphaMode;    // +0x0c AlphaMode → OpenGLTexture::GetFormat (0 for RGB/BGR formats)
        UInt32 mipmapMode;   // +0x10 MipmapMode
        Bitmap bitmap;       // +0x14 the pixels (format: see OpenGLTexture.cpp's table)
        UInt32 filter;       // +0x34 Filter (default 1)
    };

    // Image file → 2D texture (cached per path in the factory, Mac 0x2268b5).
    // Mac vtable 0x766ea8 {D1 0x33d30, D0 0x53100}; inline ctor (weak copy Mac 0x52fc0).
    // Mac sizeof 0x68, Win 0x5c.
    struct LoadArgs2D : public ObjectFactoryArgs {
        LoadArgs2D() : useColorKey(false) {}                          // Mac 0x52fc0
        virtual ~LoadArgs2D() {}                                      // Mac 0x33d30/0x53100
        CreateArgs2D create;     // +0x04 (copied into a CreateArgs2D by the factory)
        Path         path;       // +0x3c (Mac 0x18 bytes, Win 0x0c)
        Color        colorKey;   // Mac +0x54, Win +0x48: pixels of this colour become transparent
        bool         useColorKey;// Mac +0x64, Win +0x58 (Win loader only; the Mac ignores both)
    };

    // FBO colour target (RenderControl Mac 0x1540e0, post effects 0x35ace0). Mac vtable 0x7609a8
    // {D1 0x46210, D0 0x55b20}. Mac sizeof 0x1c. Users set {0x40000420, true, 1, 0, w, h}.
    // Read by Mac 0xf9790.
    struct RenderTargetCreateArgs2D : public ObjectFactoryArgs {
        RenderTargetCreateArgs2D()
            : format(0x40000420), depthBuffer(true), wrapMode(kWrapRepeat), alphaMode(0), width(0),
              height(0) {}
        virtual ~RenderTargetCreateArgs2D() {}
        UInt32 format;       // +0x04 BitmapFormat of the colour buffer (0x40000420 in every user)
        bool   depthBuffer;  // +0x08 attach a (shared) depth buffer of the same size
        UInt32 wrapMode;     // +0x0c
        UInt32 alphaMode;    // +0x10
        UInt32 width;        // +0x14 (rounded up to a power of two)
        UInt32 height;       // +0x18
    };

    // FBO colour targets for GL_ARB_draw_buffers (Mac 0xf9f40). Mac RTTI only. Layout as
    // RenderTargetCreateArgs2D + count.
    struct MultipleRenderTargetCreateArgs2D : public ObjectFactoryArgs {
        MultipleRenderTargetCreateArgs2D()
            : format(0x40000420), depthBuffer(true), wrapMode(kWrapRepeat), alphaMode(0), width(0),
              height(0), count(1) {}
        virtual ~MultipleRenderTargetCreateArgs2D() {}
        UInt32 format;       // +0x04
        bool   depthBuffer;  // +0x08
        UInt32 wrapMode;     // +0x0c
        UInt32 alphaMode;    // +0x10
        UInt32 width;        // +0x14
        UInt32 height;       // +0x18
        UInt32 count;        // +0x1c colour attachments (at most 4)
    };

    // FBO depth texture (Mac 0xfa5a0); created by the factory's depth-buffer cache (Mac 0x227680:
    // {1, 0, w, h, 2}). Mac vtable 0x770b8c {D1 0x2279a0, D0 0x228950}. Mac sizeof 0x18.
    struct DepthBufferCreateArgs2D : public ObjectFactoryArgs {
        DepthBufferCreateArgs2D()
            : wrapMode(kWrapClampToEdge), field08(0), width(0), height(0), depthFormat(2) {}
        virtual ~DepthBufferCreateArgs2D() {}                         // Mac 0x2279a0/0x228950
        UInt32 wrapMode;     // +0x04
        UInt32 field08;      // +0x08 (0; not read)
        UInt32 width;        // +0x0c
        UInt32 height;       // +0x10
        UInt32 depthFormat;  // +0x14 GL_DEPTH_COMPONENT16 + n (0: 16, 1: 24, 2: 32 bits)
    };

    // CPU-writable 2D texture (PlatformTextLayout, Mac 040). Mac vtable 0x763e30
    // {D1 0x1017d0, D0 0x102560}. Read by Mac 0xf9160. Mac sizeof 0x1c.
    struct MutableCreateArgs2D : public ObjectFactoryArgs {
        MutableCreateArgs2D()
            : priority(0.0f), wrapMode(kWrapRepeat), alphaMode(0), width(0), height(0),
              format(kBitmapFormatRGBA32) {}
        virtual ~MutableCreateArgs2D() {}
        Real32 priority;     // +0x04
        UInt32 wrapMode;     // +0x08
        UInt32 alphaMode;    // +0x0c (2 in PlatformTextLayout)
        UInt32 width;        // +0x10
        UInt32 height;       // +0x14
        UInt32 format;       // +0x18 BitmapFormat of the CPU bitmap
    };

    // CPU-writable cube map (Mac 0xfb020). Mac RTTI only. Same layout as MutableCreateArgs2D.
    struct MutableCreateArgsCube : public ObjectFactoryArgs {
        MutableCreateArgsCube()
            : priority(0.0f), wrapMode(kWrapRepeat), alphaMode(0), width(0), height(0),
              format(kBitmapFormatRGBA32) {}
        virtual ~MutableCreateArgsCube() {}
        Real32 priority;     // +0x04
        UInt32 wrapMode;     // +0x08
        UInt32 alphaMode;    // +0x0c
        UInt32 width;        // +0x10
        UInt32 height;       // +0x14
        UInt32 format;       // +0x18
    };

    // Cube map from six bitmaps (+X, -X, +Y, -Y, +Z, -Z; Mac 0xfa8f0). Mac RTTI only.
    struct CreateArgsCube : public ObjectFactoryArgs {
        CreateArgsCube()
            : priority(0.0f), wrapMode(kWrapRepeat), alphaMode(0), mipmapMode(kMipmapNone)
        {
            for (UInt32 i = 0; i < 6; ++i)
                faces[i].m_depth = 1;
        }
        virtual ~CreateArgsCube() {}
        Real32 priority;     // +0x04
        UInt32 wrapMode;     // +0x08
        UInt32 alphaMode;    // +0x0c
        UInt32 mipmapMode;   // +0x10
        Bitmap faces[6];     // +0x14, +0x34, ... +0xb4 (the format of face 0 is used for all)
    };

    // 3D texture (Mac 0xfb620). Mac RTTI only.
    struct CreateArgs3D : public ObjectFactoryArgs {
        CreateArgs3D()
            : priority(0.0f), wrapMode(kWrapRepeat), alphaMode(0), width(0), height(0), depth(0),
              mipmapMode(kMipmapNone) {}
        virtual ~CreateArgs3D() {}
        Real32 priority;     // +0x04
        UInt32 wrapMode;     // +0x08 (S, T and R)
        UInt32 alphaMode;    // +0x0c
        UInt32 width;        // +0x10
        UInt32 height;       // +0x14
        UInt32 depth;        // +0x18
        UInt32 mipmapMode;   // +0x1c (only selects compression)
        Bitmap bitmap;       // +0x20 (m_width/m_height/m_depth slices, rows of m_pitch bytes)
    };

    Texture() {}
    virtual ~Texture() {}                                             // Mac 0xfc030/0xfc0a0

    // ---- Mac slots 3-8 / Win 0-5 ----
    virtual UInt32 GetType() const = 0;                               // Mac 3 (Type)
    // OpenGLTexture2DMutable / CubeMutable override (Mac 0x227cf0 / 0x228350). (name inferred)
    virtual bool IsMutable() const = 0;                               // Mac 4
    // The alphaMode of the creation arguments (0 for formats without alpha). (name inferred)
    virtual UInt32 GetFormat() const = 0;                             // Mac 5 (OpenGL: +0x2c)
    // The CPU bitmap of a mutable texture (cube face `face`), null otherwise
    // (Mac 0xfc800 / 0x227d00 / 0x228360). (name inferred)
    virtual RefPointer<BitmapObject> GetBitmap(UInt32 face) const = 0; // Mac 6
    // Size of the source image / requested size.
    virtual Vector2 GetSize() const = 0;                              // Mac 7 (OpenGL: +0x30)
    // Size of the GL texture: the source size rounded up to powers of two. (name kept from the
    // API header; "image" = the GL texture image)
    virtual Vector2 GetImageSize() const = 0;                         // Mac 8 (OpenGL: +0x38)
    // (Mac slot 2 is the inherited NotifierT<Texture, TextureObserver>::OnFinalRelease 0xfc120;
    // OpenGLTexture overrides it, Mac 0xf9020: MakeCurrent, release GL object, detach observers.)
};

} // namespace LEGO
