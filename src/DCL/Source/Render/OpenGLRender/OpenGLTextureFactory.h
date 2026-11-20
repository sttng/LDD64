// LEGO Digital Designer — restored source
// LEGO::OpenGLTextureFactory — the render's factory for Texture: Texture::*Args -> OpenGL
// textures; image textures cached per path, FBO depth buffers shared per size. TU 168 (Mac
// 0x226870-0x2290ff; Win CreateObject 0x4f6ff0, dtor 0x4ec1a0, sdd 0x4ec220, image loader
// 0x4f6c20/0x4f6d90, mutable texture methods 0x4f5b70-0x4f6620), also the mutable / cube methods.
// Made by OpenGLRender::Initialize (Mac 0xf2fc6, 0x40 bytes). Vtables Mac 0x72dcf0 {D1 0x2279b0,
// D0 0x2279c0, .., CreateObject 0x226870, Initialize 0x19070, Shutdown 0x19080}; Win 0xa25fd4
// {sdd 0x4ec220, .., CreateObject 0x4f6ff0, ..}. Image files: Mac via ImageIO (premultiplied BGRA,
// alphaMode 2), Win via the BitmapCodecs (alphaMode from the format, optional colour key).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "BasicMap.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Path.h"
#include "Texture.h"

namespace LEGO {

class OpenGLRender;
class OpenGLTexture2D;
class OpenGLTexture2DDepthBuffer;

class OpenGLTextureFactory : public SpecificObjectFactory {
public:
    explicit OpenGLTextureFactory(OpenGLRender* render) : m_render(render) {}   // inlined (Mac 0xf2fc6)
    virtual ~OpenGLTextureFactory() {}                                // Mac 0x228960 (D2), Win 0x4ec1a0

    // LoadArgs2D (cached per path), CreateArgs2D, MutableCreateArgs2D, and with
    // EXT_framebuffer_object RenderTargetCreateArgs2D / DepthBufferCreateArgs2D, with
    // ARB_draw_buffers MultipleRenderTargetCreateArgs2D, then CreateArgsCube,
    // MutableCreateArgsCube, CreateArgs3D. Null when the arguments are not understood or the
    // texture cannot be created. The render is made current first.
    virtual Object* CreateObject(const ObjectFactoryArgs& args);     // Mac 0x226870, Win 0x4f6ff0

    // The depth texture shared by every render target of `size` (power-of-two sizes; created
    // on first use: DepthBufferCreateArgs2D {clamp, w, h, 32 bit}). Mac 0x227680.
    OpenGLTexture2DDepthBuffer* GetDepthBuffer(const Vector2& size);
    // Called by the depth texture when it releases its GL object. Mac 0x2275c0.
    void RemoveDepthBuffer(const Vector2& size);
    // Called by a cached image texture on its final release (Mac 0xf8fa0: map erase 0xfd780).
    void RemoveTexture(const Path& path);

    OpenGLRender* GetRender() const { return m_render; }

private:
    typedef Pair<Vector2, OpenGLTexture2DDepthBuffer*> DepthBufferEntry;

    // Image file → texture (the LoadArgs2D branch, Mac 0x2268b5-0x226eca).
    Object* LoadTexture(const Texture::LoadArgs2D& args);
    // Decodes the file `path` into args.bitmap (Mac: inlined ImageIO block; Win 0x4f6d90).
    // Allocates args.bitmap.m_data with new[] (the caller frees it).
    bool DecodeImageFile(const Texture::LoadArgs2D& load, Texture::CreateArgs2D& args);

    OpenGLRender*                                      m_render;        // Mac +0x10 (raw)
    BasicMap<Path, OpenGLTexture2D*>                   m_textures;      // Mac +0x14 (raw pointers: entries leave on the texture's final release)
    BasicArray<DepthBufferEntry>                       m_depthBuffers;  // Mac +0x30 (raw pointers)

    OpenGLTextureFactory(const OpenGLTextureFactory&);
    OpenGLTextureFactory& operator=(const OpenGLTextureFactory&);
};

} // namespace LEGO
