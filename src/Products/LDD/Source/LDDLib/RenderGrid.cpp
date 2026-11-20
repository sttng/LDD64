// LEGO Digital Designer — restored source
// RenderGrid.cpp (Mac TU 117, Win 0x654417-0x6559e9; LDDLib). See RenderGrid.h.
//
// Floating point: the Win build evaluates on the x87 (53-bit) and rounds at stores; the
// expressions below use EvalReal (MathUtil.h) exactly where the two builds round differently.
// The world scale 1.6 is a float literal in the Mac build (0x636a00) and the double
// (double)1.6f in the Win build (0xa2d8d0), so `(EvalReal)x * kWorldScale` is the same product.
#include "RenderGrid.h"
#include "ObjectFactory.h"
#include "Platform.h"
#include "Path.h"
#include "Directory.h"
#include "Texture.h"
#include "RenderQueue.h"
#include "MathUtil.h"

namespace LEGO {

namespace {

const Real32 kWorldScale   = 1.6f;      // grid units -> world units (Mac 0x636a00, Win 0xa2d8d0)
const Real32 kLineSpacing  = 4.0f;      // cells per line (Mac 0x633954, Win 0xa2d620)
const Real32 kBelowOpacity = 0.25f;     // Mac 0x3e800000, Win 0x9d6614
const Real32 kMaxExtent    = 300000.0f; // Mac 0x636a0c, Win 0x9d66ec
const UInt32 kStepDistance = 30000;     // squared view distance per level-of-detail step

// LEGO::RoundToNearest (Win 0x4c9630, another TU; inlined on Mac): ties go up.
inline Real32 RoundToNearest(Real32 x)
{
    Real32 f = (Real32)floor((double)x);
    Real32 c = (Real32)ceil((double)x);
    return (x - f < c - x) ? f : c;
}

// Mac: fisttp qword + low word; Win: _ftol2 (0x6cd726) — both truncate through 64 bits.
inline UInt32 ToUInt32(Real32 x)
{
    return (UInt32)(Int64)x;
}

// Win 0x40b4f0 / 0x40b4c0: min/max returning the second argument on ties (LTCG copies).
inline Real32 MinB(Real32 a, Real32 b) { return b <= a ? b : a; }
inline Real32 MaxB(Real32 a, Real32 b) { return b >= a ? b : a; }

// The plane height is never above 0: `0 < height ? 0 : height` (Mac minss, Win 0x655062).
inline Real32 PlaneY(Real32 height)
{
    return 0.0f < height ? 0.0f : height;
}

// Win 0x50fa90 (helper shared with other users); inlined on Mac.
VertexBuffer* CreateVertexBuffer(Render* render, const VertexBuffer::CreateArgs& args)
{
    Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    return object ? dynamic_cast<VertexBuffer*>(object) : 0;
}

} // namespace

// ===========================================================================================
// GridMaterial / GridTexturedSemitransparentMaterial
// ===========================================================================================

// Mac 0x1d2bd0, Win 0x654479
void GridMaterial::Begin(Render* render)
{
    render->EnableLighting(false);
}

// Mac 0x1d2bf0, Win 0x6099d6
void GridMaterial::End(Render* render)
{
    render->EnableLighting(true);
}

// Mac 0x1d2ce0, Win 0x6544ca
void GridTexturedSemitransparentMaterial::Begin(Render* render)
{
    render->SetOpacity(kBelowOpacity);
    DecalMaterial::Begin(render);
}

// Mac 0x1d2d10, Win 0x6544f3
void GridTexturedSemitransparentMaterial::End(Render* render)
{
    render->SetOpacity(1.0f);
    DecalMaterial::End(render);
}

// ===========================================================================================
// RenderGrid
// ===========================================================================================

// Mac 0x1d3020, Win 0x654b56
RenderGrid::RenderGrid()
    : m_lineMaterial(new GridMaterial),
      m_lineColor(0.4f, 0.4f, 0.4f, 1.0f),
      m_color2(0.6f, 0.6f, 0.6f, 1.0f),
      m_cellSize(0.4f),
      m_baseRect(Vector2(-8.0f, -8.0f), 16.0f, 16.0f),
      m_rect(m_baseRect),
      m_builtRect(m_baseRect),
      m_dynamic(true),
      m_step(1),
      m_stepChanged(false)
{
    m_cameraHeight = 0.0f;   // not initialised by the original (RenderGrid.h); fixed (low)
}

// Mac 0x1d3310, Win 0x655021
VertexBuffer* RenderGrid::CreateLineVertexBuffer(Render* render, const Color& color,
                                                 const Rect& rect, UInt32 step)
{
    UInt32 width = ToUInt32(RoundToNearest(rect.GetWidth()));
    UInt32 height = ToUInt32(RoundToNearest(rect.GetHeight()));
    UInt32 columns = width / step + 1;   // lines along z
    UInt32 rows = height / step + 1;     // lines along x
    UInt32 count = (columns + rows) * 2;
    Real32 y = PlaneY(m_height);

    VertexBuffer::CreateArgs args;
    args.vertexCount = count;
    args.indexCount = count;
    args.frameCount = 0;
    args.dynamic = m_dynamic;
    args.hasNormals = false;
    args.colorSetCount = 1;
    VertexBuffer* buffer = CreateVertexBuffer(render, args);
    if (!buffer)
        return 0;
    buffer->SetPrimitiveType(1);   // lines

    // Positions: rows first (constant z), then columns (constant x).
    Real32* positions = new Real32[count * 3];
    Real32 spacing = (Real32)((EvalReal)m_cellSize * kLineSpacing * (EvalReal)step);
    UInt32 n = 0;
    Real32 offset = 0.0f;
    for (UInt32 i = 0; i < rows; ++i) {
        positions[n++] = (Real32)((EvalReal)rect.m_min.x * kWorldScale);
        positions[n++] = y;
        positions[n++] = (Real32)((EvalReal)rect.m_min.y * kWorldScale + offset);
        positions[n++] = (Real32)((EvalReal)rect.GetTopRight().x * kWorldScale);
        positions[n++] = y;
        positions[n++] = (Real32)((EvalReal)rect.GetTopRight().y * kWorldScale + offset);
        offset = offset + spacing;
    }
    offset = 0.0f;
    for (UInt32 i = 0; i < columns; ++i) {
        positions[n++] = (Real32)((EvalReal)rect.m_min.x * kWorldScale + offset);
        positions[n++] = y;
        positions[n++] = (Real32)((EvalReal)rect.m_min.y * kWorldScale);
        positions[n++] = (Real32)((EvalReal)rect.GetBottomLeft().x * kWorldScale + offset);
        positions[n++] = y;
        positions[n++] = (Real32)((EvalReal)rect.GetBottomLeft().y * kWorldScale);
        offset = offset + spacing;
    }
    buffer->SetPositions((const Vector3*)positions, 0, count, 0);
    delete[] positions;

    UInt32* indices = new UInt32[count];
    for (UInt32 i = 0; i < count; ++i)
        indices[i] = i;
    buffer->SetIndices(indices, 0, count, 0);
    delete[] indices;

    Color* colors = new Color[count];
    n = 0;
    for (UInt32 i = 0; i < rows; ++i) {
        colors[n++] = color;
        colors[n++] = color;
    }
    for (UInt32 i = 0; i < columns; ++i) {
        colors[n++] = color;
        colors[n++] = color;
    }
    buffer->SetColors(colors, 0, count, 0);
    delete[] colors;
    return buffer;
}

// Mac 0x1d3240, Win 0x65578a
void RenderGrid::CreateLineMesh(Render* render, const Color& color, Material* material,
                                const Rect& rect, UInt32 step)
{
    VertexBuffer* buffer = CreateLineVertexBuffer(render, color, rect, step);
    m_lineMesh = buffer ? new SingleMesh(material, buffer) : 0;
}

// Mac 0x1d3be0, Win 0x65534d
VertexBuffer* RenderGrid::CreatePlaneVertexBuffer(Render* render, Real32 cameraHeight,
                                                  const Rect& rect, UInt32 step)
{
    (void)step;
    UInt32 width = ToUInt32(RoundToNearest(rect.GetWidth()));
    UInt32 height = ToUInt32(RoundToNearest(rect.GetHeight()));
    Real32 y = PlaneY(m_height);

    VertexBuffer::CreateArgs args;
    args.vertexCount = 4;
    args.indexCount = 4;
    args.frameCount = 0;
    args.dynamic = m_dynamic;
    args.hasNormals = false;
    args.colorSetCount = 0;
    args.texCoordDimensions.PushBack(2);
    VertexBuffer* buffer = CreateVertexBuffer(render, args);
    if (!buffer)
        return 0;
    buffer->SetPrimitiveType(4);   // triangle strip

    Real32 left = (Real32)((EvalReal)rect.m_min.x * kWorldScale);
    Real32 top = (Real32)((EvalReal)rect.m_min.y * kWorldScale);
    Real32 right = (Real32)((EvalReal)rect.m_max.x * kWorldScale);
    Real32 bottom = (Real32)((EvalReal)rect.m_max.y * kWorldScale);
    BasicArray<Vector3> positions;
    // NOTE: a NaN height takes the first branch on Mac and the second on Win.
    if (cameraHeight < m_height || cameraHeight == m_height) {
        // seen from below
        positions.PushBack(Vector3(left, y, bottom));
        positions.PushBack(Vector3(left, y, top));
        positions.PushBack(Vector3(right, y, top));
        positions.PushBack(Vector3(right, y, bottom));
    } else {
        positions.PushBack(Vector3(right, y, bottom));
        positions.PushBack(Vector3(right, y, top));
        positions.PushBack(Vector3(left, y, top));
        positions.PushBack(Vector3(left, y, bottom));
    }
    buffer->SetPositions(&positions[0], 0, 4, 0);
    const UInt32 indices[4] = { 0, 1, 3, 2 };
    buffer->SetIndices(indices, 0, 4, 0);

    Real32 u = (Real32)((EvalReal)(Real32)width * 2.0f);
    Real32 v = (Real32)((EvalReal)(Real32)height * 2.0f);
    BasicArray<Vector2> texCoords;
    texCoords.PushBack(Vector2(u, v));
    texCoords.PushBack(Vector2(u, 0.0f));
    texCoords.PushBack(Vector2(0.0f, 0.0f));
    texCoords.PushBack(Vector2(0.0f, v));
    buffer->SetTexCoords((const Real32*)&texCoords[0], 0);
    return buffer;
}

// Mac 0x1d3b20, Win 0x6557eb
void RenderGrid::CreatePlaneMesh(Render* render, Real32 cameraHeight, Material* material,
                                 const Rect& rect, UInt32 step)
{
    VertexBuffer* buffer = CreatePlaneVertexBuffer(render, cameraHeight, rect, step);
    m_planeMesh = buffer ? new SingleMesh(material, buffer) : 0;
}

// Mac 0x1d44d0, Win 0x655850
void RenderGrid::Draw(Render* render, UInt32 queueIndex)
{
    RenderQueue* queue = render->GetRenderQueue(queueIndex);
    Vector3 eye;
    Matrix3D camera;
    camera.SetInverse(render->GetViewMatrix());
    eye = camera.GetTranslation();

    // Rebuild when asked to, or when the camera crossed the plane since the last rebuild.
    if (m_dirty || m_stepChanged ||
        (m_height > m_cameraHeight && m_height < eye.y) ||
        (m_height < m_cameraHeight && m_height > eye.y)) {
        m_lineMesh = 0;
        m_planeMesh = 0;
        if (m_step < 2) {
            Material* material = (eye.y < m_height || eye.y == m_height) ? m_planeMaterialBelow
                                                                          : m_planeMaterial;
            CreatePlaneMesh(render, eye.y, material, m_rect, m_step);
        }
        CreateLineMesh(render, m_lineColor, m_lineMaterial, m_rect, m_step);
        m_dirty = false;
        m_stepChanged = false;
        m_cameraHeight = eye.y;
    }

    if (m_lineMesh) {
        UInt32 layer = (m_height <= eye.y && eye.y != m_height) ? 1000 : (UInt32)-1000;
        queue->AddTransparent(m_lineMesh, Matrix3D::Identity(), 0.0f, layer, 1.0f);
    }
    if (m_planeMesh) {
        UInt32 layer = (m_height <= eye.y && eye.y != m_height) ? 1000 : (UInt32)-1000;
        queue->AddTransparent(m_planeMesh, Matrix3D::Identity(), 0.0f, layer, 1.0f);
    }
}

// Mac 0x1d46e0, Win 0x654ce1
bool RenderGrid::Initialize(Render* render)
{
    Texture::LoadArgs2D args;
    args.path.SetElement(Platform::GetInstance()->GetSystemDirectory(9, true));
    args.path.Append(String("Graphics/General/Grid.png"));
    args.create.alphaMode = Texture::kAlphaNone;
    Object* object = render->GetObjectFactory()->CreateObject(typeid(Texture), args);
    RefPointer<Texture> texture = object ? dynamic_cast<Texture*>(object) : 0;
    if (!texture)
        return false;

    MaterialColors white;
    white.diffuse = Color::White();
    white.ambient = Color::White();
    white.specular = Color::White();
    white.emissive = Color::White();
    white.shininess = 0.0f;
    m_planeMaterial = new TexturedMaterial(white, texture);
    if (!m_planeMaterial)
        return false;

    // NOTE: both builds fill a second, all-(1,1,1,1) MaterialColors and then pass a default
    // constructed one (Color() is white too, shininess 0) — the same values.
    MaterialColors colors;
    colors.shininess = 0.0f;
    m_planeMaterialBelow = new GridTexturedSemitransparentMaterial(colors, texture);
    return m_planeMaterialBelow != 0;
}

// ===========================================================================================
// DynamicGrid
// ===========================================================================================

// Win 0x65466a (cameraPosition unused); inlined in Mac 0x1d51d0.
void DynamicGrid::UpdateStep(const Vector3& cameraPosition, const Vector3& viewVector)
{
    (void)cameraPosition;
    UInt32 level = ToUInt32(viewVector.GetLengthSquared()) / kStepDistance;
    m_stepChanged = level != m_step;
    if (level == 0) {
        m_step = 1;
        m_stepChanged = false;
    } else if (m_stepChanged) {
        UInt32 step = 1;
        while (step < level)
            step *= 2;
        m_step = step < 4 ? step : 4;
    }
}

// Mac 0x1d51d0, Win 0x65477e
void DynamicGrid::Update(const Vector3& cameraPosition, const Vector3& viewVector,
                         BoundingBox& box, const BoundingBox* extraBox, bool flag)
{
    (void)flag;
    if (extraBox)
        box |= *extraBox;
    UpdateStep(cameraPosition, viewVector);

    Real32 height = m_followBox ? box.m_min.y : 0.0f;
    if (height != m_height)
        m_dirty = true;
    m_height = height;

    // The base area (in world units) united with the scene box seen from above.
    Rect boxRect(box.m_min.x, box.m_min.z, box.m_max.x, box.m_max.z);
    Rect area((Real32)((EvalReal)m_baseRect.m_min.x * kWorldScale),
              (Real32)((EvalReal)m_baseRect.m_min.y * kWorldScale),
              (Real32)((EvalReal)m_baseRect.m_max.x * kWorldScale),
              (Real32)((EvalReal)m_baseRect.m_max.y * kWorldScale));
    Rect united(area |= boxRect);

    // Widen to whole steps: the min side downwards, the max side upwards.
    Real32 stepSize = (Real32)((EvalReal)(Real32)m_step * kWorldScale);
    Real32 restLeft = (Real32)fmod((double)united.m_min.x, (double)stepSize);
    Real32 restTop = (Real32)fmod((double)united.m_min.y, (double)stepSize);
    Real32 restRight = (Real32)fmod((double)united.m_max.x, (double)stepSize);
    Real32 restBottom = (Real32)fmod((double)united.m_max.y, (double)stepSize);
    Real32 left = restLeft < -EPSILON
        ? (Real32)((EvalReal)united.m_min.x - ((EvalReal)(Real32)m_step * kWorldScale + restLeft))
        : united.m_min.x;
    Real32 top = restTop < -EPSILON
        ? (Real32)((EvalReal)united.m_min.y - ((EvalReal)(Real32)m_step * kWorldScale + restTop))
        : united.m_min.y;
    Real32 right = EPSILON < restRight
        ? (Real32)(((EvalReal)(Real32)m_step * kWorldScale - restRight) + united.m_max.x)
        : united.m_max.x;
    Real32 bottom = EPSILON < restBottom
        ? (Real32)(((EvalReal)(Real32)m_step * kWorldScale - restBottom) + united.m_max.y)
        : united.m_max.y;

    m_rect.Set(MaxB(left / kWorldScale, -kMaxExtent), MaxB(top / kWorldScale, -kMaxExtent),
               MinB(right / kWorldScale, kMaxExtent), MinB(bottom / kWorldScale, kMaxExtent));
    if (m_builtRect != m_rect) {
        m_dirty = true;
        m_builtRect.Set(m_rect);
    }
}

} // namespace LEGO
