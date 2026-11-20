// LEGO Digital Designer — restored source
// TU 414 (Mac 0x391900-0x3943db; Win 0x5bb650-0x5bd00f). See RenderVisualDebugger.h.
// BasicArray instances (BasicArray.h): VDBShape dtor Mac 0x393a60 / 0x393bc0, insert 0x393d20;
// Category dtor 0x393300 / 0x393af0, push 0x393790; Win 0x5bcf40 / 0x5bcff0 / 0x5bcfb0.
#include "RenderVisualDebugger.h"

#include "ObjectFactory.h"
#include "Render.h"
#include "RenderQueue.h"
#include "VertexBuffer.h"
#include "Material.h"
#include "Mesh.h"
#include "Thread.h"

#include <typeinfo>

namespace LEGO {

namespace {

// Removes shape `index`: the last shape takes its place (Mac 0x392aa0: an unordered erase —
// the element-wise move of the rest that follows it in the code never runs).
void EraseShape(BasicArray<RenderVisualDebugger::VDBShape>& shapes, UInt32 index)
{
    if (index + 1 != shapes.GetSize())
        shapes[index] = shapes.Back();
    shapes.PopBack();
}

// A vertex buffer for `vertexCount` / `indexCount` coloured line vertices (one colour set, no
// normals, no texture coordinates) of `primitiveType` from the render's factory.
VertexBuffer* CreateLineBuffer(Render* render, UInt32 vertexCount, UInt32 indexCount,
                               UInt32 primitiveType)
{
    VertexBuffer::CreateArgs args;
    args.vertexCount = vertexCount;
    args.indexCount = indexCount;
    args.frameCount = 0;
    args.dynamic = false;
    args.hasNormals = false;
    args.colorSetCount = 1;
    Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    VertexBuffer* buffer = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    // NOTE (both builds): not checked for null.
    buffer->SetPrimitiveType(primitiveType);
    return buffer;
}

} // namespace

// Mac 0x393120 (C2 with VTT), Win 0x5bbea0
RenderVisualDebugger::RenderVisualDebugger(Render* render, Material* material)
    : m_render(render), m_material(material)
{
}

// Mac 0x3935c0 / 0x393680 (body 0x3933b0: mutex, then the categories), Win 0x5bbfe0 / sdd
// 0x5bbfb0
RenderVisualDebugger::~RenderVisualDebugger()
{
}

// Mac 0x393770 (thunk 0x393780)
void RenderVisualDebugger::OnFinalRelease()
{
}

// Mac 0x391a70, Win 0x5bcbf0
RenderVisualDebugger::VDBShape* RenderVisualDebugger::GetShape(UInt32 category, UInt32 id)
{
    Category* found = 0;
    for (BasicArray<Category>::Iterator it = m_categories.Begin(); it != m_categories.End(); ++it) {
        if (it->first == category) {
            found = &*it;
            break;
        }
    }
    if (found == 0) {
        m_categories.PushBack(Category(category, BasicArray<VDBShape>()));
        found = &m_categories.Back();
    }
    BasicArray<VDBShape>& shapes = found->second;
    VDBShape* shape;
    if (id == 0) {
        // the next id: largest + 1 (1 for an empty category); always a new shape
        UInt32 largest = 0;
        for (shape = shapes.Begin(); shape != shapes.End(); ++shape)
            if (shape->id > largest)
                largest = shape->id;
        id = largest + 1;
    } else {
        for (shape = shapes.Begin(); shape != shapes.End() && shape->id != id; ++shape) {
        }
    }
    if (shape == shapes.End()) {
        VDBShape added;
        added.id = id;
        const UInt32 index = (UInt32)(shape - shapes.Begin());
        shapes.Insert(shape, 1, added);
        shape = shapes.Begin() + index;
    }
    return shape;
}

// Mac 0x391cd0, Win 0x5bc140
void RenderVisualDebugger::BuildPointMesh(VDBShape& shape, const Vector3& position,
                                          const Color& color, Real32 size)
{
    VertexBuffer* buffer = CreateLineBuffer(m_render, 8, 8, 1);   // lines
    const Real32 minus = -size;
    const Vector3 positions[8] = {
        position + Vector3(size, size, 0.0f),   position + Vector3(minus, minus, 0.0f),
        position + Vector3(size, minus, 0.0f),  position + Vector3(minus, size, 0.0f),
        position + Vector3(size, 0.0f, size),   position + Vector3(minus, 0.0f, minus),
        position + Vector3(size, 0.0f, minus),  position + Vector3(minus, 0.0f, size),
    };
    const Color colors[8] = {color, color, color, color, color, color, color, color};
    const UInt32 indices[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    buffer->SetPositions(positions, 0, 8, 0);
    buffer->SetColors(colors, 0, 8, 0);
    buffer->SetIndices(indices, 0, 8, 0);
    shape.mesh = new SingleMesh(m_material, buffer);
    shape.pending = VDBShape::kNone;
}

// Mac 0x392370, Win 0x5bc6a0
void RenderVisualDebugger::BuildLineMesh(VDBShape& shape, const Vector3& from, const Vector3& to,
                                         const Color& color)
{
    VertexBuffer* buffer = CreateLineBuffer(m_render, 2, 2, 1);   // lines
    const Vector3 positions[2] = {from, to};
    const Color colors[2] = {color, color};
    const UInt32 indices[2] = {0, 1};
    buffer->SetPositions(positions, 0, 2, 0);
    buffer->SetColors(colors, 0, 2, 0);
    buffer->SetIndices(indices, 0, 2, 0);
    shape.mesh = new SingleMesh(m_material, buffer);
    shape.pending = VDBShape::kNone;
}

// Mac 0x3926f0, Win 0x5bc8d0
void RenderVisualDebugger::BuildTriangleMesh(VDBShape& shape, const Vector3& a, const Vector3& b,
                                             const Vector3& c, const Color& color)
{
    VertexBuffer* buffer = CreateLineBuffer(m_render, 3, 4, 2);   // line strip
    const Vector3 positions[3] = {a, b, c};
    const Color colors[3] = {color, color, color};
    const UInt32 indices[4] = {0, 1, 2, 0};
    buffer->SetPositions(positions, 0, 3, 0);
    buffer->SetColors(colors, 0, 3, 0);
    buffer->SetIndices(indices, 0, 4, 0);
    // NOTE (both builds): the outline has no material, unlike points and lines.
    shape.mesh = new SingleMesh(0, buffer);
    shape.pending = VDBShape::kNone;
}

// Mac 0x3919a0, Win 0x5bb6c0
UInt32 RenderVisualDebugger::AddPoint(UInt32 category, UInt32 id, const Vector3& position,
                                      const Color& color, Real32 size)
{
    m_mutex.Lock();
    VDBShape* shape = GetShape(category, id);
    if (!Thread::IsMainThread()) {
        shape->pending = VDBShape::kPoint;
        shape->a = position;
        shape->color = color;
        shape->size = size;
    } else {
        BuildPointMesh(*shape, position, color, size);
    }
    const UInt32 result = shape->id;
    m_mutex.Unlock();
    return result;
}

// Mac 0x3922a0, Win 0x5bb7c0
UInt32 RenderVisualDebugger::AddLine(UInt32 category, UInt32 id, const Vector3& from,
                                     const Vector3& to, const Color& color)
{
    m_mutex.Lock();
    VDBShape* shape = GetShape(category, id);
    if (!Thread::IsMainThread()) {
        shape->pending = VDBShape::kLine;
        shape->a = from;
        shape->b = to;
        shape->color = color;
    } else {
        BuildLineMesh(*shape, from, to, color);
    }
    const UInt32 result = shape->id;
    m_mutex.Unlock();
    return result;
}

// Mac 0x392600, Win 0x5bb8e0
UInt32 RenderVisualDebugger::AddTriangle(UInt32 category, UInt32 id, const Vector3& a,
                                         const Vector3& b, const Vector3& c, const Color& color)
{
    m_mutex.Lock();
    VDBShape* shape = GetShape(category, id);
    if (!Thread::IsMainThread()) {
        shape->pending = VDBShape::kTriangle;
        shape->a = a;
        shape->b = b;
        shape->c = c;
        shape->color = color;
    } else {
        BuildTriangleMesh(*shape, a, b, c, color);
    }
    const UInt32 result = shape->id;
    m_mutex.Unlock();
    return result;
}

// Mac 0x3929e0, Win 0x5bba30
void RenderVisualDebugger::RemoveShape(UInt32 category, UInt32 id)
{
    m_mutex.Lock();
    for (BasicArray<Category>::Iterator it = m_categories.Begin(); it != m_categories.End(); ++it) {
        if (it->first != category)
            continue;
        BasicArray<VDBShape>& shapes = it->second;
        for (UInt32 i = 0; i < shapes.GetSize(); ++i) {
            if (shapes[i].id != id)
                continue;
            if (!Thread::IsMainThread())
                shapes[i].pending = VDBShape::kRemove;
            else
                EraseShape(shapes, i);
            break;
        }
        break;
    }
    m_mutex.Unlock();
}

// Mac 0x392c60, Win 0x5bbb00
UInt32 RenderVisualDebugger::NewShape(UInt32 category)
{
    m_mutex.Lock();
    const UInt32 result = GetShape(category, 0)->id;
    m_mutex.Unlock();
    return result;
}

// Mac 0x392cd0, Win 0x5bbb70
void RenderVisualDebugger::OnSceneEvent(UInt32 code)
{
    m_mutex.Lock();
    for (BasicArray<Category>::Iterator it = m_categories.Begin(); it != m_categories.End(); ++it) {
        if (it->first != code)
            continue;
        if (!Thread::IsMainThread()) {
            for (BasicArray<VDBShape>::Iterator s = it->second.Begin(); s != it->second.End(); ++s)
                s->pending = VDBShape::kRemove;
        } else {
            it->second.Clear();
        }
        break;
    }
    m_mutex.Unlock();
}

// Mac 0x392db0, Win 0x5bbbf0
void RenderVisualDebugger::RemoveAllShapes()
{
    m_mutex.Lock();
    for (BasicArray<Category>::Iterator it = m_categories.Begin(); it != m_categories.End(); ++it) {
        if (!Thread::IsMainThread()) {
            for (BasicArray<VDBShape>::Iterator s = it->second.Begin(); s != it->second.End(); ++s)
                s->pending = VDBShape::kRemove;
        } else {
            it->second.Clear();
        }
    }
    m_mutex.Unlock();
}

// Mac 0x392eb0, Win 0x5bbc70
UInt32 RenderVisualDebugger::SetShapeMesh(UInt32 category, UInt32 id, Mesh* mesh)
{
    m_mutex.Lock();
    VDBShape* shape = GetShape(category, id);
    // NOTE (guard): both builds reference `mesh` without a null check when it differs.
    shape->mesh = mesh;
    shape->pending = VDBShape::kNone;
    const UInt32 result = shape->id;
    m_mutex.Unlock();
    return result;
}

// Mac 0x392f60, Win 0x5bbcf0
void RenderVisualDebugger::QueueDebugMeshes(RenderQueue* queue, const Matrix3D& transform)
{
    m_mutex.Lock();
    for (BasicArray<Category>::Iterator it = m_categories.Begin(); it != m_categories.End(); ++it) {
        BasicArray<VDBShape>& shapes = it->second;
        UInt32 i = 0;
        while (i < shapes.GetSize()) {
            VDBShape& shape = shapes[i];
            switch (shape.pending) {
            case VDBShape::kPoint:
                BuildPointMesh(shape, shape.a, shape.color, shape.size);
                break;
            case VDBShape::kLine:
                BuildLineMesh(shape, shape.a, shape.b, shape.color);
                break;
            case VDBShape::kTriangle:
                BuildTriangleMesh(shape, shape.a, shape.b, shape.c, shape.color);
                break;
            case VDBShape::kRemove:
                EraseShape(shapes, i);
                continue;
            default:
                break;
            }
            if (!shape.mesh.IsNull())
                queue->Add(shape.mesh, transform, 0.0f, 0, 1.0f);
            ++i;
        }
    }
    m_mutex.Unlock();
}

} // namespace LEGO
