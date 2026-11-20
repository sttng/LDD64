// LEGO Digital Designer — restored source
// Steps the effects (TU 369 / 370 / 371) repeat inline in Apply / Initialize / Resize. Not
// original declarations: each helper names the code it stands for. (names inferred)
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "Color.h"
#include "LEGO/String.h"
#include "ObjectFactory.h"
#include "Render.h"
#include "RenderQueue.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "Material.h"
#include "Directory.h"
#include "Path.h"

#include <typeinfo>

namespace LEGO {
namespace PostProcessing {

// A full-screen pass over `size`: saves scissor, viewport, projection and view, then viewport
// and scissor = (0, 0, size), view = identity, projection = orthographic over that rectangle
// (z -1 .. 1), culling and depth test off. End restores them (culling back faces, depth test
// on). E.g. Mac 0x35b8b8-0x35b9e2 / 0x35c1a8-0x35c26c (SSAO Apply).
class ScreenPass {
public:
    ScreenPass(Render* render, const Vector2& size)
        : m_render(render),
          m_scissor(render->GetScissor()),
          m_viewport(render->GetViewport()),
          m_projection(render->GetProjectionMatrix()),
          m_view(render->GetViewMatrix())
    {
        const Rect rect(size);
        render->SetViewport(rect);
        render->SetScissor(rect);
        Matrix3D matrix;
        render->SetViewMatrix(matrix);
        matrix.SetOrthographic(rect, -1.0f, 1.0f);
        render->SetProjectionMatrix(matrix);
        render->SetCullMode(Render::kCullNone);
        render->EnableDepthTest(false);
    }
    void End()
    {
        m_render->SetCullMode(Render::kCullBack);
        m_render->EnableDepthTest(true);
        m_render->SetScissor(m_scissor);
        m_render->SetViewport(m_viewport);
        m_render->SetViewMatrix(m_view);
        m_render->SetProjectionMatrix(m_projection);
    }

private:
    Render*  m_render;
    Rect     m_scissor;
    Rect     m_viewport;
    Matrix3D m_projection;
    Matrix3D m_view;
};

// A clamped render target with its own depth buffer (Texture::RenderTargetCreateArgs2D
// {format, true, 1, 0, w, h}); null when the factory refuses. E.g. Mac 0x35b668-0x35b7a0.
inline Texture* CreateTarget(Render* render, UInt32 format, const Vector2& imageSize)
{
    Texture::RenderTargetCreateArgs2D args;
    args.format = format;
    args.depthBuffer = true;
    args.wrapMode = Texture::kWrapClampToEdge;
    args.alphaMode = 0;
    args.width = (UInt32)(Int64)imageSize.x;
    args.height = (UInt32)(Int64)imageSize.y;
    Object* object = render->GetObjectFactory()->CreateObject(typeid(Texture), args);
    return object ? dynamic_cast<Texture*>(object) : 0;
}

// The image `name` of the shader directory (Texture::LoadArgs2D, alpha mode 2). E.g. Mac
// 0x35b3f6-0x35b4c5 (SSAO Initialize).
inline Texture* LoadShaderTexture(Render* render, Directory* shaders, const String& name)
{
    Texture::LoadArgs2D args;
    args.path.SetElement(shaders);
    args.path.Append(name);
    args.create.alphaMode = Texture::kAlphaImage;
    Object* object = render->GetObjectFactory()->CreateObject(typeid(Texture), args);
    return object ? dynamic_cast<Texture*>(object) : 0;
}

// The quad's second texture-coordinate set: (0 0) (1 0) (0 1) (1 1). E.g. Mac 0x35b531-0x35b56f.
inline void SetUnitTexCoords(VertexBuffer* quad)
{
    const Real32 coordinates[8] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    quad->SetTexCoords(coordinates, 1);
}

// The scene queue once more with the normals material into `normalsTarget`, which shares the
// depth buffer of `target`: grey (0.8, 0.8, 0.8, 0) background, colour and stencil cleared
// (depth kept), unsorted, the queue's materials replaced by the default material. E.g. Mac
// 0x35d55c-0x35d64c (SceneNormals Apply), 0x35f141-0x35f24a (SSDO Apply).
inline void DrawNormals(Render* render, RenderQueue* queue, Texture* normalsTarget, Texture* target,
                        Material* material)
{
    render->SetRenderTargetDepthBuffer(normalsTarget, render->GetRenderTargetDepthBuffer(target));
    render->SetRenderTarget(normalsTarget);
    render->SetClearColor(Color(0.8f, 0.8f, 0.8f, 0.0f));
    render->Clear(Render::kClearColor | Render::kClearStencil);
    render->SetDefaultMaterial(material);
    queue->Flush(0.02f, false, false);
    render->SetDefaultMaterial(0);
}

} // namespace PostProcessing
} // namespace LEGO
