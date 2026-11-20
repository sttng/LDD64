// LEGO Digital Designer — restored source
// LDDUtilities (Mac TU 063) — the rotation bands of the hinge tool: CreateRingVertexBuffer and
// CreateSelectableMesh (SelectableMesh.h). One object with the rest of LDDUtilities in the
// original (see LDDUtilities.h).
#include "LDDUtilities.h"
#include "SelectableMesh.h"
#include "VertexBuffer.h"
#include "Render.h"
#include "ObjectFactory.h"
#include "Color.h"
#include "MathUtil.h"

#include <typeinfo>

namespace LEGO {

// Mac 0x14d5f0, Win 0x637f14
VertexBuffer* CreateRingVertexBuffer(Render* render, const Color& color, bool front, Real32 radius,
                                     Real32 unused, UInt32 segments)
{
    (void)color;
    (void)unused;
    const UInt32 count = segments * 2 + 2;
    VertexBuffer::CreateArgs args;
    args.vertexCount = count;
    args.indexCount = count;
    args.texCoordDimensions.PushBack(2);
    Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    VertexBuffer* buffer = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    if (!buffer)
        return 0;
    buffer->SetPrimitiveType(4);

    Vector3* positions = new Vector3[count];
#if defined(_WIN32)
    const Real32 bottom = (Real32)(-(double)radius * 0.5);       // Win: fchs; fmul 0.5 (double)
    const Real32 segmentCount = (Real32)(double)segments;
    const Real32 step = (Real32)((double)(2.0f * PI) / segmentCount);   // Win float 0xaef298
    const Real32 top = (Real32)((double)bottom + radius);
#else
    const Real32 bottom = -0.5f * radius;
    const Real32 segmentCount = (Real32)segments;
    const Real32 step = (2.0f * PI) / segmentCount;
    const Real32 top = radius + bottom;
#endif
    for (UInt32 i = 0; i <= segments; ++i) {
#if defined(_WIN32)
        const Real32 angle = (Real32)((double)(i * 2) * step);
#else
        const Real32 angle = (Real32)(i * 2) * step;
#endif
        const Real32 s = Sin(angle);
        const Real32 c = Cos(angle);
        positions[i * 2]     = Vector3(c, bottom, s);
        positions[i * 2 + 1] = Vector3(c, top, s);
    }
    buffer->SetPositions(positions, 0, count, 0);
    delete[] positions;

    Vector2* texCoords = new Vector2[count];
    const Real32 left  = front ? 0.0f : 0.5f;
    const Real32 right = front ? 0.5f : 1.0f;
#if defined(_WIN32)
    const Real32 texStep = (Real32)(1.0 / ((double)segmentCount * 0.25));
#else
    const Real32 texStep = 1.0f / (segmentCount * 0.25f);
#endif
    for (UInt32 i = 0; i <= segments; ++i) {
#if defined(_WIN32)
        const Real32 t = (Real32)((double)i * texStep);
#else
        const Real32 t = (Real32)i * texStep;
#endif
        texCoords[i * 2]     = Vector2(left, t);
        texCoords[i * 2 + 1] = Vector2(right, t);
    }
    buffer->SetTexCoords((Real32*)texCoords[0], 0);
    delete[] texCoords;

    UInt32* indices = new UInt32[count];
    for (UInt32 i = 0; i < count; ++i)
        indices[i] = i;
    buffer->SetIndices(indices, 0, count, 0);
    delete[] indices;
    return buffer;
}

// Mac 0x14db80, Win 0x638220
SelectableMesh* CreateSelectableMesh(Render* render, const Color& color, Material* material,
                                     bool front, Real32 radius, Real32 unused, UInt32 segments)
{
    VertexBuffer* buffer = CreateRingVertexBuffer(render, color, front, radius, unused, segments);
    if (!buffer)
        return 0;
    return new SelectableMesh(material, buffer);
}

} // namespace LEGO
