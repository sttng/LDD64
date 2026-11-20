// LEGO Digital Designer — restored source
// TU 178 (file name inferred): the 9-slice image of StretchableToggleControl.
#include "StretchableImage.h"
#include "Control.h"
#include "ControlManager.h"
#include "Render.h"
#include "ObjectFactory.h"

#include <typeinfo>

namespace LEGO {

// Two triangles per quad of the 4 × 4 point grid, row by row. Mac 0x770dd0, Win 0xaeab68
static UInt32 s_indices[54] = {
    0, 1, 4,   1, 5, 4,     1, 2, 5,   2, 6, 5,     2, 3, 6,   3, 7, 6,
    4, 5, 8,   5, 9, 8,     5, 6, 9,   6, 10, 9,    6, 7, 10,  7, 11, 10,
    8, 9, 12,  9, 13, 12,   9, 10, 13, 10, 14, 13,  10, 11, 14, 11, 15, 14
};

// Mac 0x233f90, Win 0x51c237
bool StretchableImage::Create(Texture* texture, const Vector3& columns, const Vector3& rows,
                              ControlManager* manager)
{
    m_texture = texture;
    m_columns = columns;
    m_rows = rows;

    VertexBuffer::CreateArgs args;
    args.vertexCount = 16;
    args.indexCount = 54;
    args.frameCount = 0;
    args.dynamic = true;
    args.hasNormals = false;
    args.colorSetCount = 0;
    args.texCoordDimensions.PushBack(2);
    Object* object = manager->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    m_vertexBuffer = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    if (!m_vertexBuffer)
        return false;
    m_vertexBuffer->SetIndices(s_indices, 0, 54, 0);
    m_vertexBuffer->SetPrimitiveType(3);
    return true;
}

// Mac 0x234170, Win 0x51c380
void StretchableImage::SetImagePosition(const Vector2& imageOrigin)
{
    Real32 x2 = m_columns.x + m_columns.y;
    Real32 x3 = m_columns.z + x2;
    Real32 y2 = m_rows.x + m_rows.y;
    Real32 y3 = m_rows.z + y2;
    Vector2 points[16] = {
        Vector2(0.0f, 0.0f),     Vector2(m_columns.x, 0.0f),     Vector2(x2, 0.0f),     Vector2(x3, 0.0f),
        Vector2(0.0f, m_rows.x), Vector2(m_columns.x, m_rows.x), Vector2(x2, m_rows.x), Vector2(x3, m_rows.x),
        Vector2(0.0f, y2),       Vector2(m_columns.x, y2),       Vector2(x2, y2),       Vector2(x3, y2),
        Vector2(0.0f, y3),       Vector2(m_columns.x, y3),       Vector2(x2, y3),       Vector2(x3, y3)
    };
    Vector2 imageSize = m_texture->GetImageSize();
    for (UInt32 i = 0; i < 16; ++i) {
        Vector2 p = imageOrigin + points[i];
        p.Divide(imageSize.x, imageSize.y);
        points[i] = p;
    }
    m_vertexBuffer->SetTexCoords(&points[0].x, 0);
}

// Mac 0x2344d0, Win 0x51c4ec
void StretchableImage::SetSize(const Vector2& size)
{
    Real32 x2 = size.x - m_columns.z;
    Real32 y2 = size.y - m_columns.z;   // NOTE: m_columns.z, not m_rows.z (both builds)
    Vector3 positions[16] = {
        Vector3(0.0f, 0.0f, 0.0f),     Vector3(m_columns.x, 0.0f, 0.0f),     Vector3(x2, 0.0f, 0.0f),     Vector3(size.x, 0.0f, 0.0f),
        Vector3(0.0f, m_rows.x, 0.0f), Vector3(m_columns.x, m_rows.x, 0.0f), Vector3(x2, m_rows.x, 0.0f), Vector3(size.x, m_rows.x, 0.0f),
        Vector3(0.0f, y2, 0.0f),       Vector3(m_columns.x, y2, 0.0f),       Vector3(x2, y2, 0.0f),       Vector3(size.x, y2, 0.0f),
        Vector3(0.0f, size.y, 0.0f),   Vector3(m_columns.x, size.y, 0.0f),   Vector3(x2, size.y, 0.0f),   Vector3(size.x, size.y, 0.0f)
    };
    m_vertexBuffer->SetPositions(positions, 0, 16, 0);
}

// Mac 0x234800
void StretchableImage::Draw(ControlDrawArgs& args)
{
    Render* render = args.m_render;
    render->EnableBlend(m_texture->GetFormat() != 0);
    render->SetTexture(m_texture.Get(), 0);
    render->SetVertexBuffer(m_vertexBuffer.Get());
    render->DrawVertexBuffer();
}

} // namespace LEGO
