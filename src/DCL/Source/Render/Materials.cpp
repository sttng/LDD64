// LEGO Digital Designer — restored source
// Materials.cpp (Mac TU 043; name inferred) + the material members the original emitted inline
// in TU 022b (BrickConnectSceneManager.cpp). See Materials.h.
#include "Materials.h"
#include "ObjectFactory.h"
#include "Path.h"
#include "Directory.h"
#include "MathUtil.h"

namespace LEGO {

namespace {

const Real32 kOne = 1.0f;   // Mac 0x62fff8

// Mac 0x107d40, Win 0x5bac10: a clamped texture with image alpha from the shader directory.
Texture* LoadShaderTexture(Render* render, const String& name, Directory* directory)
{
    Texture::LoadArgs2D args;
    args.path = Path(name, directory);
    args.create.wrapMode = Texture::kWrapClampToEdge;
    args.create.alphaMode = Texture::kAlphaImage;
    Object* object = render->GetObjectFactory()->CreateObject(typeid(Texture), args);
    return object ? dynamic_cast<Texture*>(object) : 0;
}

// Uniform 1 / width of the texture bound on `unit` (inlined in four Begin functions).
void SetTexelOffset(Render* render, Shader* shader, UInt32 unit, const char* name)
{
    Vector2 size = render->GetTexture(unit)->GetImageSize();
    shader->SetVariable(AString(name), kOne / size.x, false);
}

} // namespace

// ===========================================================================================
// ColoredMaterial
// ===========================================================================================

// Mac 0x109b20, Win 0x57c9a0
bool ColoredMaterial::IsOpaque() const
{
    return m_colors.diffuse.a == 1.0f;
}

// Mac 0x109b50, Win 0x57ba30
bool ColoredMaterial::IsDecal() const
{
    return false;
}

// Mac 0x106c40, Win 0x5ba120
void ColoredMaterial::Begin(Render* render)
{
    render->EnableLighting(true);
    render->SetLocalViewer(true);
    render->SetMaterialColors(m_colors);
}

// Mac 0x106c80, Win 0x5ba160
void ColoredMaterial::End(Render* render)
{
    render->SetLocalViewer(false);
    render->EnableLighting(false);
}

// ===========================================================================================
// FilledMaterial
// ===========================================================================================

// Mac 0x109c30, Win 0x6123e8
bool FilledMaterial::IsOpaque() const
{
    return m_colors.diffuse.a == 1.0f;
}

// Mac 0x109c60, Win 0x67e76a
bool FilledMaterial::IsDecal() const
{
    return false;
}

// Mac 0x106cb0, Win 0x5ba180
void FilledMaterial::Begin(Render* render)
{
    render->EnableLighting(false);
    render->SetMaterialColors(m_colors);
    render->SetCullMode(Render::kCullNone);
}

// Mac 0x106cf0, Win 0x5ba1c0
void FilledMaterial::End(Render* render)
{
    render->SetCullMode(Render::kCullBack);
}

// ===========================================================================================
// WireframeMaterial
// ===========================================================================================

// Mac 0x106d10, Win 0x5ba1e0
void WireframeMaterial::Begin(Render* render)
{
    render->SetPolygonOffset(-1.0f, 0.0f);
    render->EnablePolygonOffset(true);
    render->SetPolygonMode(1);
    render->EnableLighting(true);
    render->SetLocalViewer(true);
    render->SetMaterialColors(m_colors);
}

// Mac 0x106d80, Win 0x5ba230
void WireframeMaterial::End(Render* render)
{
    render->SetPolygonMode(2);
    render->EnablePolygonOffset(false);
    render->SetLocalViewer(false);
    render->EnableLighting(false);
}

// ===========================================================================================
// TexturedMaterial and subclasses
// ===========================================================================================

// Mac 0x109e10, Win 0x5bd890
bool TexturedMaterial::IsOpaque() const
{
    if (m_texture->GetFormat() != 0)
        return false;
    return m_colors.diffuse.a == 1.0f;
}

// Mac 0x106dd0, Win 0x5ba260
void TexturedMaterial::Begin(Render* render)
{
    render->SetTexture(m_texture, 0);
    render->EnableLighting(true);
    render->SetLocalViewer(true);
    render->SetMaterialColors(m_colors);
}

// Mac 0x106e30, Win 0x5ba290
void TexturedMaterial::End(Render* render)
{
    render->SetTexture(0, 0);
    render->SetLocalViewer(false);
    render->EnableLighting(false);
}

// Mac 0x106e70, Win 0x5ba2c0
void ReverseWindingTexturedMaterial::Begin(Render* render)
{
    render->SetFrontFace(0);
    TexturedMaterial::Begin(render);   // inlined in the original
}

// Mac 0x106ed0, Win 0x5ba2e0
void ReverseWindingTexturedMaterial::End(Render* render)
{
    render->SetFrontFace(1);
    TexturedMaterial::End(render);     // inlined in the original
}

// Mac 0x10a250, Win 0x5fbd39
bool DecalMaterial::IsDecal() const
{
    return true;
}

// Mac 0x106f20, Win 0x5ba300
void DecalMaterial::Begin(Render* render)
{
    render->SetPolygonOffset(0.0f, -1.0f);
    render->EnablePolygonOffset(true);
    TexturedMaterial::Begin(render);   // inlined in the original
}

// Mac 0x106fa0, Win 0x5ba340
void DecalMaterial::End(Render* render)
{
    render->EnablePolygonOffset(false);
    TexturedMaterial::End(render);     // inlined in the original
}

// Mac 0x106ff0, Win 0x5ba360
void TexturedSkyboxMaterial::Begin(Render* render)
{
    render->EnableDepthWrite(false);
    render->EnableLighting(false);
    TexturedMaterial::Begin(render);   // inlined; NOTE: turns lighting back on
}

// Mac 0x107060, Win 0x5ba390
void TexturedSkyboxMaterial::End(Render* render)
{
    render->EnableDepthWrite(true);
    render->EnableLighting(true);
    TexturedMaterial::End(render);     // inlined in the original
}

// ===========================================================================================
// ShadedMaterial family
// ===========================================================================================

// Mac 0x63cb0 (TU 022b), Win 0x5b9060
ShadedMaterial::ShadedMaterial(Render* render, Shader* shader, const MaterialColors& colors)
    : m_shader(shader), m_colors(colors)
{
    m_opacity = render->GetOpacity();
    m_shader->SetMaterialColors(colors);
}

// Mac 0x638e0, Win 0x5b9200
bool ShadedMaterial::IsOpaque() const
{
    return m_shader->IsOpaque();
}

// Mac 0x63720, Win 0x4aede0
bool ShadedMaterial::IsDecal() const
{
    return false;
}

// Mac 0x1070c0, Win 0x5ba3c0
void ShadedMaterial::Begin(Render* render)
{
    Real32 difference = m_opacity - render->GetOpacity();
    // `!(x < eps)`: a NaN difference also re-uploads (ucomiss / fcomp both fall through).
    if (!(Abs(difference) < 1e-05f)) {
        m_opacity = render->GetOpacity();
        Real32 alpha = m_colors.diffuse.a * m_opacity;
        m_shader->SetVariable(AString("DiffuseColor"), m_colors.diffuse.r, m_colors.diffuse.g,
                              m_colors.diffuse.b, alpha, false);
    }
    render->SetShader(m_shader);
}

// Mac 0x107250, Win 0x5bb470
void ShadedMaterial::End(Render* render)
{
    render->SetShader(0);
}

// Mac 0x63550 (TU 022b)
ShadedTexturedMaterial::ShadedTexturedMaterial(Render* render, Shader* shader,
                                               const MaterialColors& colors, Texture* texture)
    : ShadedMaterial(render, shader, colors), m_texture(texture)
{
    shader->SetVariable(AString("Texture0"), (Int32)0, false);
}

// Mac 0x63700, Win 0x60a87d
bool ShadedTexturedMaterial::IsOpaque() const
{
    return m_shader->IsOpaque();
}

// Mac 0x107530, Win 0x5ba640
void ShadedTexturedMaterial::Begin(Render* render)
{
    m_shader->SetVariable(AString("Texture0"), (Int32)0, false);
    render->SetTexture(m_texture, 0);
    ShadedMaterial::Begin(render);
}

// Mac 0x107650, Win 0x5ba6e0
void ShadedTexturedMaterial::End(Render* render)
{
    render->SetTexture(0, 0);
    render->SetShader(0);
}

// Mac 0x63ae0, Win 0x67e76a
bool ShadedDecalMaterial::IsOpaque() const
{
    return false;
}

// Mac 0x63af0, Win 0x5fbd39
bool ShadedDecalMaterial::IsDecal() const
{
    return true;
}

// Mac 0x1072f0, Win 0x5ba5a0
void ShadedColorChangeMaterial::Begin(Render* render)
{
    m_shader->SetMaterialColors(render->GetMaterialColors());
    ShadedMaterial::Begin(render);
}

// Mac 0x107f30 / 0x107680, Win 0x5ba710
ShadedRoundEdgeMaterial::ShadedRoundEdgeMaterial(Render* render, Shader* shader,
                                                 const MaterialColors& colors,
                                                 Directory* shaderDirectory)
    : ShadedMaterial(render, shader, colors)
{
    m_texture1 = LoadShaderTexture(render, String("blue.png"), shaderDirectory);
    m_texture2 = LoadShaderTexture(render, String("red.png"), shaderDirectory);
    m_texture3 = LoadShaderTexture(render, String("magenta.png"), shaderDirectory);
    m_shader->SetVariable(AString("Texture0"), (Int32)1, false);
    m_shader->SetVariable(AString("Texture1"), (Int32)2, false);
    m_shader->SetVariable(AString("Texture2"), (Int32)3, false);
    m_shader->SetVariable(AString("VertexTexture"), (Int32)4, false);
    m_shader->SetVariable(AString("PackedAverageNormalTexture"), (Int32)5, false);
    m_shader->SetVariable(AString("PackedRoundEdgeDataTexture"), (Int32)6, false);
}

// Mac 0x10a660, Win 0x5b9200
bool ShadedRoundEdgeMaterial::IsOpaque() const
{
    return m_shader->IsOpaque();
}

// Mac 0x108600, Win 0x5baa40
void ShadedRoundEdgeMaterial::Begin(Render* render)
{
    if (render->GetTexture(4) && render->GetTexture(6)) {
        render->SetTexture(m_texture1, 1);
        render->SetTexture(m_texture2, 2);
        render->SetTexture(m_texture3, 3);
        SetTexelOffset(render, m_shader, 4, "vertexNormalTexelOffset");
        SetTexelOffset(render, m_shader, 6, "packedRedTexelOffset");
    }
    ShadedMaterial::Begin(render);
}

// Mac 0x1088d0, Win 0x5babc0
void ShadedRoundEdgeMaterial::End(Render* render)
{
    render->SetTexture(0, 1);
    render->SetTexture(0, 2);
    render->SetTexture(0, 3);
    render->SetShader(0);
}

// Mac 0x1089c0, Win 0x5badc0
void ShadedRoundEdgeColorChangeMaterial::Begin(Render* render)
{
    m_shader->SetMaterialColors(render->GetMaterialColors());
    ShadedRoundEdgeMaterial::Begin(render);
}

// Mac 0x108a00, Win 0x5bae00
ShadedRoundEdgeDecalMaterial::ShadedRoundEdgeDecalMaterial(Render* render, Shader* shader,
                                                           const MaterialColors& colors,
                                                           Texture* decal)
    : ShadedMaterial(render, shader, colors), m_decal(decal)
{
    m_shader->SetVariable(AString("Texture0"), (Int32)0, false);
    m_shader->SetVariable(AString("VertexTexture"), (Int32)4, false);
    m_shader->SetVariable(AString("PackedAverageNormalTexture"), (Int32)5, false);
    m_shader->SetVariable(AString("PackedRoundEdgeDataTexture"), (Int32)6, false);
}

// Mac 0x108dd0, Win 0x5bafc0
bool ShadedRoundEdgeDecalMaterial::IsOpaque() const
{
    return m_decal->GetFormat() == 0;
}

// Mac 0x108df0, Win 0x4f5b70
bool ShadedRoundEdgeDecalMaterial::IsDecal() const
{
    return true;
}

// Mac 0x108e00, Win 0x5bafd0
void ShadedRoundEdgeDecalMaterial::Begin(Render* render)
{
    if (render->GetTexture(4))
        SetTexelOffset(render, m_shader, 4, "vertexNormalTexelOffset");
    render->SetTexture(m_decal, 0);
    ShadedMaterial::Begin(render);
}

// Mac 0x108f80, Win 0x5ba6e0 (ICF with ShadedTexturedMaterial::End)
void ShadedRoundEdgeDecalMaterial::End(Render* render)
{
    render->SetTexture(0, 0);
    render->SetShader(0);
}

// Mac 0x109210, Win 0x5bb130
ShadedRoundEdgeWireframeMaterial::ShadedRoundEdgeWireframeMaterial(Render* render, Shader* shader,
                                                                   const MaterialColors& colors)
    : ShadedMaterial(render, shader, colors)
{
    m_shader->SetVariable(AString("VertexTexture"), (Int32)4, false);
}

// Mac 0x1093a0, Win 0x5bb250
void ShadedRoundEdgeWireframeMaterial::Begin(Render* render)
{
    if (render->GetTexture(4))
        SetTexelOffset(render, m_shader, 4, "vertexNormalTexelOffset");
    render->SetPolygonMode(1);
    ShadedMaterial::Begin(render);
}

// Mac 0x109510, Win 0x5bb320
void ShadedRoundEdgeWireframeMaterial::End(Render* render)
{
    render->SetPolygonMode(2);
    render->SetShader(0);
}

// Mac 0x109740, Win 0x5bb3a0
SceneNormalsMaterial::SceneNormalsMaterial(Render* render, Shader* shader)
    : m_shader(shader)
{
    (void)render;
}

// Mac 0x10a9e0, Win 0x5b9200
bool SceneNormalsMaterial::IsOpaque() const
{
    return m_shader->IsOpaque();
}

// Mac 0x10aa00, Win 0x4aede0
bool SceneNormalsMaterial::IsDecal() const
{
    return false;
}

// Mac 0x1097c0, Win 0x5bb450
void SceneNormalsMaterial::Begin(Render* render)
{
    render->SetShader(m_shader);
}

// Mac 0x1097e0, Win 0x5bb470
void SceneNormalsMaterial::End(Render* render)
{
    render->SetShader(0);
}

// Mac 0x109800, Win 0x5bb490
SceneNormalsRoundEdgeMaterial::SceneNormalsRoundEdgeMaterial(Render* render, Shader* shader)
    : SceneNormalsMaterial(render, shader)
{
    m_shader->SetVariable(AString("VertexTexture"), (Int32)4, false);
}

// Mac 0x109990, Win 0x5bb560
void SceneNormalsRoundEdgeMaterial::Begin(Render* render)
{
    if (render->GetTexture(4))
        SetTexelOffset(render, m_shader, 4, "vertexNormalTexelOffset");
    render->SetShader(m_shader);
}

// Mac 0x109b00, Win 0x5bb630
void SceneNormalsRoundEdgeMaterial::End(Render* render)
{
    render->SetShader(0);
}

// ===========================================================================================
// Helper materials (TU 022b)
// ===========================================================================================

bool SilhouetteMaterial::IsOpaque() const { return true; }            // Mac 0x62ff0, Win 0x5fbd39
bool SilhouetteMaterial::IsDecal() const { return false; }            // Mac 0x63000, Win 0x67e76a

// Mac 0x63010, Win 0x609782
void SilhouetteMaterial::Begin(Render* render)
{
    render->SetTexture(0, 0);
    render->EnableLighting(false);
    render->SetPolygonMode(1);
    render->SetLineWidth(2);
    render->SetColor(Color::Black());
}

// Mac 0x63080, Win 0x6097d0
void SilhouetteMaterial::End(Render* render)
{
    render->SetLineWidth(1);
    render->SetPolygonMode(2);
    render->EnableLighting(true);
}

bool OutlineMaterial::IsOpaque() const { return true; }               // Mac 0x63190, Win 0x5fbd39
bool OutlineMaterial::IsDecal() const { return false; }               // Mac 0x631a0, Win 0x67e76a

// Mac 0x631b0, Win 0x60999a
void OutlineMaterial::Begin(Render* render)
{
    render->SetTexture(0, 0);
    render->EnableLighting(false);
    render->SetColor(Color::Black());
}

// Mac 0x63200, Win 0x6099d6
void OutlineMaterial::End(Render* render)
{
    render->EnableLighting(true);
}

bool CoordinateSystemMaterial::IsOpaque() const { return true; }      // Mac 0x63400, Win 0x5fbd39
bool CoordinateSystemMaterial::IsDecal() const { return false; }      // Mac 0x63410, Win 0x67e76a

// Mac 0x63420, Win 0x6098ac
void CoordinateSystemMaterial::Begin(Render* render)
{
    render->EnableLighting(false);
    render->SetLineWidth(2);
}

// Mac 0x63450, Win 0x6098c7
void CoordinateSystemMaterial::End(Render* render)
{
    render->SetLineWidth(1);
    render->EnableLighting(true);
}

// ===========================================================================================
// Vertex-buffer builders
// ===========================================================================================

namespace {

VertexBuffer* CreateLineBuffer(Render* render, UInt32 vertexCount, UInt32 indexCount, bool dynamic)
{
    VertexBuffer::CreateArgs args;
    args.vertexCount = vertexCount;
    args.indexCount = indexCount;
    args.frameCount = 0;
    args.dynamic = dynamic;
    args.hasNormals = false;
    args.colorSetCount = 1;
    Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    return object ? dynamic_cast<VertexBuffer*>(object) : 0;
}

} // namespace

// Mac 0x106320, Win 0x5b94d0 (positions Mac 0x632310, indices 0x632370)
VertexBuffer* CreateBoxVertexBuffer(Render* render, const Color& color, bool dynamic)
{
    static const Real32 kPositions[24] = {
        0, 0, 0,  1, 0, 0,  1, 1, 0,  0, 1, 0,
        0, 0, 1,  1, 0, 1,  1, 1, 1,  0, 1, 1
    };
    static const UInt32 kIndices[24] = {
        0, 1, 1, 2, 2, 3, 3, 0,  4, 5, 5, 6, 6, 7, 7, 4,  0, 4, 1, 5, 2, 6, 3, 7
    };
    VertexBuffer* buffer = CreateLineBuffer(render, 8, 24, dynamic);
    if (!buffer)
        return 0;
    buffer->SetPrimitiveType(1);
    Real32 positions[24];
    for (UInt32 i = 0; i < 24; ++i)
        positions[i] = kPositions[i];
    buffer->SetPositions((const Vector3*)positions, 0, 8, 0);
    UInt32 indices[24];
    for (UInt32 i = 0; i < 24; ++i)
        indices[i] = kIndices[i];
    buffer->SetIndices(indices, 0, 24, 0);
    Color colors[8];
    for (UInt32 i = 0; i < 8; ++i)
        colors[i] = color;
    buffer->SetColors(colors, 0, 8, 0);
    return buffer;
}

// Mac 0x1065a0, Win 0x5b9920
SingleMesh* CreateBoxMesh(Render* render, const Color& color, Material* material, bool dynamic)
{
    VertexBuffer* buffer = CreateBoxVertexBuffer(render, color, dynamic);
    if (!buffer)
        return 0;
    return new SingleMesh(material, buffer);
}

// Mac 0x106610, Win 0x5b99c0. The vertices are the compile-time cos/sin values of 0°, 120°
// and 240° (bit patterns 0xbf000001, 0x3f5db3d7, 0xbefffffd, 0xbf5db3d8).
VertexBuffer* CreateTriangleVertexBuffer(Render* render, const Color& color)
{
    VertexBuffer* buffer = CreateLineBuffer(render, 3, 3, false);
    if (!buffer)
        return 0;
    buffer->SetPrimitiveType(4);
    Vector3* positions = new Vector3[3];
    positions[0].x = 1.0f;         positions[0].y = 0.0f; positions[0].z = 0.0f;
    positions[1].x = -0.50000006f; positions[1].y = 0.0f; positions[1].z = 0.866025388f;
    positions[2].x = -0.499999911f; positions[2].y = 0.0f; positions[2].z = -0.866025448f;
    buffer->SetPositions(positions, 0, 3, 0);
    delete[] positions;
    UInt32* indices = new UInt32[3];
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    buffer->SetIndices(indices, 0, 3, 0);
    delete[] indices;
    Color* colors = new Color[3];
    colors[0] = color;
    colors[1] = color;
    colors[2] = color;
    buffer->SetColors(colors, 0, 3, 0);
    delete[] colors;
    return buffer;
}

// Mac 0x1068a0, Win 0x5b9cb0
SingleMesh* CreateTriangleMesh(Render* render, const Color& color, Material* material)
{
    VertexBuffer* buffer = CreateTriangleVertexBuffer(render, color);
    if (!buffer)
        return 0;
    return new SingleMesh(material, buffer);
}

// Mac 0x106910, Win 0x5b9d40
VertexBuffer* CreateAxesVertexBuffer(Render* render, const Color& x, const Color& y,
                                     const Color& z, bool dynamic)
{
    VertexBuffer* buffer = CreateLineBuffer(render, 6, 6, dynamic);
    if (!buffer)
        return 0;
    buffer->SetPrimitiveType(1);
    Real32 positions[18] = {
        0, 0, 0,  1, 0, 0,
        0, 0, 0,  0, 1, 0,
        0, 0, 0,  0, 0, 1
    };
    buffer->SetPositions((const Vector3*)positions, 0, 6, 0);
    UInt32 indices[6] = { 0, 1, 2, 3, 4, 5 };
    buffer->SetIndices(indices, 0, 6, 0);
    Color colors[6] = { x, x, y, y, z, z };
    buffer->SetColors(colors, 0, 6, 0);
    return buffer;
}

// Mac 0x106bc0, Win 0x5ba070
SingleMesh* CreateAxesMesh(Render* render, const Color& x, const Color& y, const Color& z,
                           Material* material, bool dynamic)
{
    VertexBuffer* buffer = CreateAxesVertexBuffer(render, x, y, z, dynamic);
    if (!buffer)
        return 0;
    return new SingleMesh(material, buffer);
}

} // namespace LEGO
