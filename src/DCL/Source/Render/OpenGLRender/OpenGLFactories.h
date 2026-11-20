// LEGO Digital Designer — restored source (restoration seam, NOT an original header)
// Factory hooks between OpenGLRender.cpp (TU 034) and resource classes restored in other files.
// OpenGLRender::Initialize (Mac slot 84 0xf2f00, Win slot 82 0x4ec850) registers, in order:
//   1. OpenGLVertexBufferFactory (0x14 bytes, TU 034)  type OpenGLVertexBuffer, abstract VertexBuffer
//   2. OpenGLTextureFactory      (0x40 bytes, TU 168)  type OpenGLTexture,      abstract Texture
//   3. FontPlatformFactory       (0x10 bytes, TU 118)  type FontPlatform,       abstract Font
//   4. OpenGLShaderFactory       (0x4c bytes, TU 400)  type OpenGLShader,       abstract Shader
//   5. PlatformTextLayoutFactory (0x14 bytes, TU 034)  type PlatformTextLayout, abstract TextLayout
//                                                      and type TextLayoutRender, abstract TextLayout
// In the original these are plain `new XFactory(render)` / constructor calls; the functions keep
// OpenGLRender.cpp independent of the other files. Each is defined by the file that restores the
// class (noted per group); the restoration's tests link stubs for the ones not restored yet.
#pragma once
#include "LEGO/Types.h"
#include "ObjectFactory.h"
#include "VertexBuffer.h"
#include <typeinfo>

namespace LEGO {

class OpenGLRender;
class Render;
class RenderQueue;

// ---- group B: textures (OpenGLTextureFactory.cpp, TU 168) ----------------------------------
// `new OpenGLTextureFactory(render)` (Mac 0xf2fc6: 0x40 bytes: render +0x10, BasicMap<Path,
// OpenGLTexture2D*> +0x14, BasicArray<Pair<Vector2, OpenGLTexture2DDepthBuffer*>> +0x30).
SpecificObjectFactory* CreateOpenGLTextureFactory(OpenGLRender* render);

// ---- group B: vertex buffers (OpenGLVertexBuffer.cpp TU 037, OpenGLVertexBufferVBO.cpp TU 039,
// OpenGLVertexBufferVBORoundEdge.cpp TU 395) -------------------------------------------------
// Each one is the constructor call + Create (OpenGLVertexBuffer slot Mac 34, +0x88) of
// OpenGLVertexBufferFactory::CreateObject (Mac 0xf7a70, Win 0x4ebe30):
//   p = new T(render, args.vertexCount, args.indexCount, args.colorSetCount, args.dynamic
//             [, roundEdgeDataCount, roundEdgePointCount]);
//   if (!p->Create(args.hasNormals, args.texCoordDimensions, args.frameCount)) { release p; p = 0; }
// and returns p (the reference the factory hands out: refcount untouched, i.e. 0 for a new
// object, exactly like the original). The choice between them stays in the factory.
VertexBuffer* CreateOpenGLVertexBuffer(OpenGLRender* render, const VertexBuffer::CreateArgs& args);       // new 0x48 (Mac 0xfdf40)
VertexBuffer* CreateOpenGLVertexBufferVBO(OpenGLRender* render, const VertexBuffer::CreateArgs& args);    // new 0x68 (Mac 0xff290)
VertexBuffer* CreateOpenGLVertexBufferVBORoundEdge(OpenGLRender* render,
                                                   const VertexBuffer::CreateArgsRoundEdge& args);       // new 0x84 (Mac 0x37e970)

// ---- group B: render queues (RenderQueue.cpp, TU 042) ----------------------------------------
// `new RenderQueue(render, capacity)` (0x70 bytes; ctor inlined at Mac 0xf4730, Win 0x4eb7f0?).
RenderQueue* CreateRenderQueue(Render* render, UInt32 capacity);

// ---- group C: shaders (OpenGLShaderFactory.cpp, TU 400) --------------------------------------
// `new OpenGLShaderFactory(render)` (0x4c bytes, ctor Mac 0x384360).
SpecificObjectFactory* CreateOpenGLShaderFactory(OpenGLRender* render);

// ---- R8 (UI): fonts and text layout (FontPlatformFactory.cpp TU 118, PlatformTextLayout.cpp TU 040)
// `new FontPlatformFactory` (0x10 bytes, no render pointer).
SpecificObjectFactory* CreateFontPlatformFactory();
// typeid() of the UI text classes the registrations name (Font, FontPlatform, TextLayout,
// PlatformTextLayout, TextLayoutRender), defined in those UI files.
const std::type_info& TypeOfFont();
const std::type_info& TypeOfFontPlatform();
const std::type_info& TypeOfTextLayout();
const std::type_info& TypeOfPlatformTextLayout();
const std::type_info& TypeOfTextLayoutRender();
// PlatformTextLayoutFactory::CreateObject (Mac 0xf7f50, Win 0x4ec310) for a
// TextLayout::CreateArgs (`args`, already dynamic_cast): `new PlatformTextLayout(render,
// args.text (+4), font (+0x14, dynamic_cast to FontPlatform, may be null), args +0x18, +0x1c,
// +0x20)` (0x68 bytes, Mac 0x100fc0). Returns null when the font is not a FontPlatform.
Object* CreatePlatformTextLayout(OpenGLRender* render, const ObjectFactoryArgs& args);

} // namespace LEGO
