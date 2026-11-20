// LEGO Digital Designer — restored source
// TU 369 "PostProcessingEffect.cpp" (Mac 0x35a9e0-0x35b36f; Win 0x5bfc10-0x5bfccf,
// 0x5c37a0-0x5c3d2f): the effect base class. See PostProcessingEffect.h.
#include "PostProcessingEffect.h"

#include "ObjectFactory.h"
#include "Render.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "Shader.h"
#include "Directory.h"
#include "Path.h"
#include "Materials.h"

#include <math.h>
#include <typeinfo>

namespace LEGO {

// Mac 0x35a9e0, Win 0x5c37a0
PostProcessingEffect::PostProcessingEffect(Render* render, Directory* shaders, bool roundEdges,
                                           Directory* roundEdgeShaders)
    : m_render(render),
      m_shaderDirectory(shaders),
      m_roundEdgeShaderDirectory(roundEdgeShaders),
      m_rect1c(0.0f, 0.0f, 0.0f, 0.0f),
      m_rect2c(0.0f, 0.0f, 0.0f, 0.0f),
      m_roundEdges(roundEdges)
{
}

// Mac 0x35b220 (D1; body 0x35b270) / 0x35b230 (D0), Win 0x5bfc10 / sdd 0x5bfcb0: the quad, the
// target and the round-edge directory are released (member destructors).
PostProcessingEffect::~PostProcessingEffect()
{
}

// Mac 0x35ab10, Win 0x5c3850
bool PostProcessingEffect::Initialize()
{
    VertexBuffer::CreateArgs args;
    args.vertexCount = 4;
    args.indexCount = 4;
    args.frameCount = 0;
    args.dynamic = false;
    args.hasNormals = false;
    args.colorSetCount = 0;
    args.texCoordDimensions.PushBack(2);
    args.texCoordDimensions.PushBack(2);
    Object* object = m_render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    m_quad = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    if (m_quad.IsNull())
        return false;
    const UInt32 indices[4] = {0, 1, 2, 3};
    m_quad->SetIndices(indices, 0, 4, 0);
    m_quad->SetPrimitiveType(4);   // triangle strip
    return true;
}

// Mac 0x35ace0, Win 0x5c3a60
void PostProcessingEffect::Resize(const Vector2& size, const Vector2& imageSize)
{
    (void)size;
    Texture::RenderTargetCreateArgs2D args;
    args.format = 0x40000420;
    args.depthBuffer = true;
    args.wrapMode = Texture::kWrapClampToEdge;
    args.alphaMode = 0;
    args.width = (UInt32)(Int64)imageSize.x;
    args.height = (UInt32)(Int64)imageSize.y;
    Object* object = m_render->GetObjectFactory()->CreateObject(typeid(Texture), args);
    m_target = object ? dynamic_cast<Texture*>(object) : 0;
}

// Mac 0x35ade0, Win 0x5c3bb0
void PostProcessingEffect::Deactivate()
{
    m_target = 0;
}

// Mac 0x35ae10, Win 0x5c3bd0
void PostProcessingEffect::Begin(const Color& clearColor)
{
    m_render->SetRenderTarget(m_target);
    m_render->SetClearColor(clearColor);
    m_render->Clear(Render::kClearColor | Render::kClearDepth | Render::kClearStencil);
}

// Mac 0x35ae60, Win 0x5c3c10
void PostProcessingEffect::SetQuadPositions(const Rect& rect)
{
    const Real32 left = floorf(rect.m_min.x);
    const Real32 top = floorf(rect.m_min.y);
    const Real32 right = floorf(rect.m_max.x);
    const Real32 bottom = floorf(rect.m_max.y);
    const Vector3 positions[4] = {
        Vector3(left, top, 0.0f), Vector3(right, top, 0.0f),
        Vector3(left, bottom, 0.0f), Vector3(right, bottom, 0.0f),
    };
    m_quad->SetPositions(positions, 0, 4, 0);
}

// Mac 0x35af80, Win 0x5c3cd0
void PostProcessingEffect::SetQuadTexCoords(const Rect& rect)
{
    const Real32 coordinates[8] = {
        rect.m_min.x, rect.m_min.y, rect.m_max.x, rect.m_min.y,
        rect.m_min.x, rect.m_max.y, rect.m_max.x, rect.m_max.y,
    };
    m_quad->SetTexCoords(coordinates, 0);
}

// (inlined in the Resize overrides, e.g. Mac 0x35b6f0-0x35b812)
void PostProcessingEffect::FitQuad(const Vector2& size, Texture* target)
{
    // the used part of the target is its top: v from 1 down to (height - used) / height
    Rect coordinates(0.0f, target->GetImageSize().y, size.x, target->GetImageSize().y - size.y);
    coordinates.Divide(target->GetImageSize());
    SetQuadPositions(Rect(size));
    SetQuadTexCoords(coordinates);
}

// Mac 0x35aff0 (Win inlined)
Shader* PostProcessingEffect::LoadShader(const String& name, Shader* current,
                                         bool fromRoundEdgeDirectory)
{
    Directory* directory = fromRoundEdgeDirectory ? m_roundEdgeShaderDirectory.Get()
                                                  : m_shaderDirectory;
    // NOTE (both builds): without the directory the result is null, not `current` — the
    // callers then drop their shader.
    if (directory == 0)
        return 0;
    Shader::LoadArgs args;
    args.path = Path(name, directory);
    args.field1C = 1;
    Object* object = m_render->GetObjectFactory()->CreateObject(typeid(Shader), args);
    Shader* shader = object ? dynamic_cast<Shader*>(object) : 0;
    if (shader != 0)
        current = shader;
    return current;
}

// (inlined in Mac 0x35d9c0, 0x35e700, 0x3603b0)
bool PostProcessingEffect::LoadNormalsMaterial(RefPointer<Shader>& shader,
                                               RefPointer<Material>& material)
{
    Material* normals;
    if (!m_roundEdges) {
        shader = LoadShader(String("Normals.sh"), shader, false);
        if (shader.IsNull())
            return false;
        normals = new SceneNormalsMaterial(m_render, shader);
    } else {
        shader = LoadShader(String("RENormals.sh"), shader, true);
        if (shader.IsNull())
            return false;
        normals = new SceneNormalsRoundEdgeMaterial(m_render, shader);
    }
    material = normals;
    return true;
}

} // namespace LEGO
