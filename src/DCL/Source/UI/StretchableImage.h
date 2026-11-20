// LEGO Digital Designer — restored source
// LEGO::StretchableImage — the 9-slice image of StretchableToggleControl: a texture, the slices'
// three column widths / row heights (image pixels) and a 16-vertex / 54-index vertex buffer (4 × 4
// points, nine quads); texture coordinates from the grid at an image position, positions from the
// control size.
// TU 178 (file name inferred: StretchableImage.cpp; Mac 0x233f90-0x2348fb), Win 0x51c237,
// 0x51c380, 0x51c4ec (Draw inlined into Win 0x519fd9). No RTTI (a plain member of
// StretchableToggleControl at Mac +0x250, Win +0x26c). All names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "Texture.h"
#include "VertexBuffer.h"

namespace LEGO {

class ControlManager;
struct ControlDrawArgs;

class StretchableImage {
public:
    StretchableImage() {}

    // Keeps `texture` and the slice sizes, creates the vertex buffer (16 vertices, 54 indices,
    // dynamic, one 2-D texture coordinate set) through the manager's object factory and sets the
    // fixed indices and primitive type 3. False when no buffer could be created.
    // Mac 0x233f90, Win 0x51c237
    bool Create(Texture* texture, const Vector3& columns, const Vector3& rows,
                ControlManager* manager);
    // Texture coordinates of the grid placed at `imageOrigin` (image pixels), divided by the
    // texture's image size. Mac 0x234170, Win 0x51c380
    void SetImagePosition(const Vector2& imageOrigin);
    // Positions of the grid for a control of `size`: corner columns / rows keep their size.
    // NOTE: original quirk: the third row starts at size.y - columns.z (the right column's
    // width), not size.y - rows.z. Mac 0x2344d0, Win 0x51c4ec
    void SetSize(const Vector2& size);
    // Blending on for textures of a non-zero format, texture unit 0, the buffer, draw.
    // Mac 0x234800 (Win inlined)
    void Draw(ControlDrawArgs& args);

    Texture*      GetTexture() const { return m_texture.Get(); }
    VertexBuffer* GetVertexBuffer() const { return m_vertexBuffer.Get(); }

private:
    Vector3                  m_columns;        // +0x00 left, centre, right widths
    Vector3                  m_rows;           // +0x0c top, centre, bottom heights
    RefPointer<Texture>      m_texture;        // +0x18
    RefPointer<VertexBuffer> m_vertexBuffer;   // +0x1c
};

} // namespace LEGO
