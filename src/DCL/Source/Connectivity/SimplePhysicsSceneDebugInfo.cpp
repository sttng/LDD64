// LEGO Digital Designer — restored source
// Scene::GetDebugInfo — the debug drawing of a scene (bodies, rigid elements, elements,
// connection fields and collision volumes as lines / triangles) into a
// SimplePhysicsSceneDebugInfo; Scene::SetListener.
//
// Original object file: Scene.cpp (Mac TU 025). This file holds sub-TU 025p (Mac
// 0xb8ec0-0xbac90: GetDebugInfo 0xb8ec0 / 0xbaa40, the body drawing 0xb8f80, SetListener
// 0xbab80), the drawing helpers the compiler placed in 025q (box 0xbfb00, sphere 0xc0580, line
// push 0xc11b0) and the inline virtuals emitted in 025s (SimplePhysicsSceneDebugInfo
// 0xc9b80-0xc9db0, InternalGetDebugInfoCollisionVolumeVisitor 0xca080-0xca1a0, triangle push
// 0xc9e50). Win: 0x4857f0 (field drawing), 0x486e10 (box), 0x487c10 (sphere), 0x488ac0
// (capsule), 0x48a3e0 / 0x48a460 (GetDebugInfo), 0x48d600-0x48d710 (visitor), 0x48d760 (body).
// (file name inferred)
//
// Matrix3D(a, b) is a * b with b applied first (Geometry.h): an element's world transform is
// Matrix3D(world of its PrimitiveRigidElement, element transform).
#include "SimplePhysicsSceneDebugInfo.h"
#include "Scene.h"
#include "Part.h"
#include "HingedElement.h"
#include "RigidElement.h"
#include "PrimitiveRigidElement.h"
#include "PrimitiveElement.h"
#include "Primitive.h"
#include "ConnectionField.h"
#include "ConnectionFieldPlanar.h"
#include "ConnectionFieldHinge.h"
#include "ConnectionFieldFixed.h"
#include "MathUtil.h"

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <libkern/OSAtomic.h>
#endif

namespace LEGO {

// ---------------------------------------------------------------------------------------------
// SimplePhysicsSceneDebugInfo
// ---------------------------------------------------------------------------------------------

namespace {
// {00000000-0000-0000-C000-000000000046} (Mac 0x762760, Win 0x46dd20)
const InterfaceID& GetIUnknownIID()
{
    static const InterfaceID id = {{0x00000000, 0x00000000, 0x000000c0, 0x46000000}};
    return id;
}
#if defined(_WIN32)
const Int32 kNoInterface = (Int32)0x80004005;   // E_FAIL (Win)
#else
const Int32 kNoInterface = (Int32)0x80000008;   // E_FAIL of the Mac COM headers
#endif
} // namespace

// Mac 0xc9b80, Win 0x46ebf0
Int32 SimplePhysicsSceneDebugInfo::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &PhysicsSceneDebugInfo::GetIID(), sizeof id) != 0 &&
        memcmp(&id, &GetIUnknownIID(), sizeof id) != 0) {
        *object = 0;
        return kNoInterface;
    }
    *object = this;
    AddRef();
    return 0;
}

// Mac 0xc9c70, Win 0x46ec50
UInt32 SimplePhysicsSceneDebugInfo::AddRef()
{
#if defined(_WIN32)
    return (UInt32)InterlockedIncrement((LONG*)&m_refCount);
#else
    return (UInt32)OSAtomicAdd32(1, (int32_t*)&m_refCount);
#endif
}

// Mac 0xc9c90, Win 0x46ec70
UInt32 SimplePhysicsSceneDebugInfo::Release()
{
#if defined(_WIN32)
    Int32 count = InterlockedDecrement((LONG*)&m_refCount);
#else
    Int32 count = OSAtomicAdd32(-1, (int32_t*)&m_refCount);
#endif
    if (count == 0) {
        // `delete this` (the destructor is not virtual; spelled out against the warning)
        this->SimplePhysicsSceneDebugInfo::~SimplePhysicsSceneDebugInfo();
        ::operator delete(this);
    }
    return (UInt32)count;
}

// ---------------------------------------------------------------------------------------------
// Drawing helpers (file-local in the original: register arguments)
// ---------------------------------------------------------------------------------------------

namespace {

const Real32 kTwoPi = 2 * PI;

// Half size of the crosses / axes drawn for the fields. NOTE: a function-local static with a
// run-time initialisation on Win (0xb44b64, guard bit 0xb44b68); a literal on Mac.
const Real32 kMarkSize = 0.1f;

// Edges or faces of `box` placed by `transform`; `color` NULL: a random colour. Faces with the
// rotated axis normals. Mac 0xbfb00 (ECX box, EDX transform), Win 0x486e10
void AddBox(const BoundingBox& box, const Matrix3D& transform, const Color* color,
            SimplePhysicsSceneDebugInfo* info, bool solid)
{
    PhysicsSceneDebugInfo::Triangle triangle;
    PhysicsSceneDebugInfo::Line line;
    triangle.m_color = color != 0 ? *color : Color::Random();
    line.m_color = triangle.m_color;

    const Vector3& lo = box.m_min;
    const Vector3& hi = box.m_max;
    const Vector3 c0 = Vector3(lo.x, lo.y, hi.z) * transform;
    const Vector3 c1 = Vector3(hi.x, lo.y, hi.z) * transform;
    const Vector3 c2 = Vector3(hi.x, hi.y, hi.z) * transform;
    const Vector3 c3 = Vector3(lo.x, hi.y, hi.z) * transform;
    const Vector3 c4 = Vector3(lo.x, lo.y, lo.z) * transform;
    const Vector3 c5 = Vector3(hi.x, lo.y, lo.z) * transform;
    const Vector3 c6 = Vector3(hi.x, hi.y, lo.z) * transform;
    const Vector3 c7 = Vector3(lo.x, hi.y, lo.z) * transform;

    if (solid) {
        // two triangles per face, the three normals of a triangle the face normal
        struct Face {
            static void Add(SimplePhysicsSceneDebugInfo* info, PhysicsSceneDebugInfo::Triangle& t,
                            const Vector3& a, const Vector3& b, const Vector3& c)
            {
                t.m_vertices[0] = a;
                t.m_vertices[1] = b;
                t.m_vertices[2] = c;
                info->m_triangles.PushBack(t);
            }
            static void SetNormal(PhysicsSceneDebugInfo::Triangle& t, const Vector3& n)
            {
                t.m_normals[0] = n;
                t.m_normals[1] = n;
                t.m_normals[2] = n;
            }
        };
        Face::SetNormal(triangle, Rotate(Vector3(0, 0, 1), transform));
        Face::Add(info, triangle, c0, c1, c2);
        Face::Add(info, triangle, c0, c2, c3);
        Face::SetNormal(triangle, Rotate(Vector3(0, 0, -1), transform));
        Face::Add(info, triangle, c6, c5, c4);
        Face::Add(info, triangle, c6, c4, c7);
        Face::SetNormal(triangle, Rotate(Vector3(-1, 0, 0), transform));
        Face::Add(info, triangle, c0, c3, c4);
        Face::Add(info, triangle, c3, c7, c4);
        Face::SetNormal(triangle, Rotate(Vector3(1, 0, 0), transform));
        Face::Add(info, triangle, c1, c5, c2);
        Face::Add(info, triangle, c2, c5, c6);
        Face::SetNormal(triangle, Rotate(Vector3(0, -1, 0), transform));
        Face::Add(info, triangle, c0, c4, c1);
        Face::Add(info, triangle, c4, c5, c1);
        Face::SetNormal(triangle, Rotate(Vector3(0, 1, 0), transform));
        Face::Add(info, triangle, c2, c6, c3);
        Face::Add(info, triangle, c6, c7, c3);
        return;
    }
    const Vector3* edges[12][2] = {
        {&c0, &c1}, {&c1, &c2}, {&c2, &c3}, {&c3, &c0},
        {&c4, &c5}, {&c5, &c6}, {&c6, &c7}, {&c7, &c4},
        {&c0, &c4}, {&c1, &c5}, {&c2, &c6}, {&c3, &c7}};
    for (UInt32 i = 0; i < 12; ++i) {
        line.m_from = *edges[i][0];
        line.m_to = *edges[i][1];
        info->m_lines.PushBack(line);
    }
}

// Segments of a tessellated circle: (1 + r)^2 + 16 rounded down to even.
UInt32 SegmentCount(Real32 radius)
{
    Real32 r = 1.0f + radius;
    return ((UInt32)(Int64)(r * r) + 16) & ~1u;
}

// The quads between two vertex rows (`segments` + 1 vertices each) as two lines (down, along
// the lower row) or two triangles. `upperCenter` / `lowerCenter`: the points the normals of the
// two rows are measured from.
void AddBand(const Vector3* upper, const Vector3* lower, UInt32 segments, const Vector3& upperCenter,
             const Vector3& lowerCenter, Real32 radius, PhysicsSceneDebugInfo::Line& line,
             PhysicsSceneDebugInfo::Triangle& triangle, SimplePhysicsSceneDebugInfo* info, bool solid)
{
    for (UInt32 j = 0; j < segments; ++j) {
        if (!solid) {
            line.m_from = upper[j];
            line.m_to = lower[j];
            info->m_lines.PushBack(line);
            line.m_from = lower[j];
            line.m_to = lower[j + 1];
            info->m_lines.PushBack(line);
            continue;
        }
        triangle.m_vertices[0] = upper[j];
        triangle.m_vertices[1] = upper[j + 1];
        triangle.m_vertices[2] = lower[j];
        triangle.m_normals[0] = (upper[j] - upperCenter) / radius;
        triangle.m_normals[1] = (upper[j + 1] - upperCenter) / radius;
        triangle.m_normals[2] = (lower[j] - lowerCenter) / radius;
        info->m_triangles.PushBack(triangle);
        triangle.m_vertices[0] = upper[j + 1];
        triangle.m_vertices[1] = lower[j + 1];
        triangle.m_vertices[2] = lower[j];
        triangle.m_normals[0] = (upper[j + 1] - upperCenter) / radius;
        triangle.m_normals[1] = (lower[j + 1] - lowerCenter) / radius;
        triangle.m_normals[2] = (lower[j] - lowerCenter) / radius;
        info->m_triangles.PushBack(triangle);
    }
}

// The cap fan of `pole` and a vertex row: lines pole - row[j + 1] (and row[j] - row[j + 1] at the
// top), or triangles (top: pole, row[j + 1], row[j]; bottom: row[j], row[j + 1], pole).
void AddCap(const Vector3& pole, const Vector3* row, UInt32 segments, bool top, const Vector3& center,
            Real32 radius, PhysicsSceneDebugInfo::Line& line, PhysicsSceneDebugInfo::Triangle& triangle,
            SimplePhysicsSceneDebugInfo* info, bool solid)
{
    for (UInt32 j = 0; j < segments; ++j) {
        if (!solid) {
            line.m_from = pole;
            line.m_to = row[j + 1];
            info->m_lines.PushBack(line);
            if (top) {
                line.m_from = row[j];
                line.m_to = row[j + 1];
                info->m_lines.PushBack(line);
            }
            continue;
        }
        if (top) {
            triangle.m_vertices[0] = pole;
            triangle.m_vertices[1] = row[j + 1];
            triangle.m_vertices[2] = row[j];
        } else {
            triangle.m_vertices[0] = row[j];
            triangle.m_vertices[1] = row[j + 1];
            triangle.m_vertices[2] = pole;
        }
        for (UInt32 k = 0; k < 3; ++k)
            triangle.m_normals[k] = (triangle.m_vertices[k] - center) / radius;
        info->m_triangles.PushBack(triangle);
    }
}

// A UV sphere: pole (0, r, 0), segments - 1 rows of segments + 1 vertices, pole (0, -r, 0).
// Mac 0xc0580 (ECX sphere, EDX transform), Win 0x487c10
void AddSphere(const Sphere& sphere, const Matrix3D& transform, const Color* color,
               SimplePhysicsSceneDebugInfo* info, bool solid)
{
    PhysicsSceneDebugInfo::Triangle triangle;
    PhysicsSceneDebugInfo::Line line;
    triangle.m_color = color != 0 ? *color : Color::Random();
    line.m_color = triangle.m_color;

    const Real32 radius = sphere.m_radius;
    const UInt32 segments = SegmentCount(radius);
    const Real32 longitudeStep = kTwoPi / (Real32)segments;
    const Real32 latitudeStep = PI / (Real32)segments;
    const UInt32 stride = segments + 1;
    const UInt32 last = (segments - 1) * stride + 1;
    Vector3* vertices = new Vector3[last + 1];
    vertices[0] = (sphere.m_center + Vector3(0, radius, 0)) * transform;
    for (UInt32 i = 1; i < segments; ++i) {
        Real32 latitude = (Real32)i * latitudeStep;
        Real32 y = Cos(latitude);
        Real32 s = Sin(latitude);
        Vector3* row = vertices + 1 + (i - 1) * stride;
        for (UInt32 j = 0; j <= segments; ++j) {
            Real32 longitude = (Real32)j * longitudeStep;
            row[j] = (sphere.m_center + Vector3(Cos(longitude) * radius * s, radius * y,
                                                s * radius * Sin(longitude))) * transform;
        }
    }
    vertices[last] = (sphere.m_center + Vector3(0, -radius, 0)) * transform;

    // NOTE: the normals are measured from the transform's origin, not from the sphere's centre
    // (right only for spheres about the origin, as the collision spheres are)
    const Vector3 origin = transform.GetTranslation();
    AddCap(vertices[0], vertices + 1, segments, true, origin, radius, line, triangle, info, solid);
    for (UInt32 r = 0; r + 2 < segments; ++r)
        AddBand(vertices + 1 + r * stride, vertices + 1 + (r + 1) * stride, segments, origin, origin, radius,
                line, triangle, info, solid);
    AddCap(vertices[last], vertices + 1 + (segments - 2) * stride, segments, false, origin, radius, line,
           triangle, info, solid);
    delete[] vertices;
}

// A capsule along x from 0 to `length`: half spheres of `radius` about both ends, joined by a
// band. Pole (-r, 0, 0); rows 1..n/2 of the first half sphere, rows n/2..n-1 shifted by
// `length` (the two equator rows make the band), pole (length + r, 0, 0).
// Mac inline in 0xca1a0, Win 0x488ac0
void AddCapsule(Real32 radius, Real32 length, const Matrix3D& transform, const Color* color,
                SimplePhysicsSceneDebugInfo* info, bool solid)
{
    PhysicsSceneDebugInfo::Triangle triangle;
    PhysicsSceneDebugInfo::Line line;
    triangle.m_color = color != 0 ? *color : Color::Random();
    line.m_color = triangle.m_color;

    const UInt32 segments = SegmentCount(radius);
    const UInt32 half = segments / 2;
    const Real32 longitudeStep = kTwoPi / (Real32)segments;
    const Real32 latitudeStep = PI / (Real32)segments;
    const UInt32 stride = segments + 1;
    const UInt32 last = segments * stride + 1;
    Vector3* vertices = new Vector3[last + 1];
    vertices[0] = Vector3(-radius, 0, 0) * transform;
    // rows 0..half-1: latitudes 1..half about 0; rows half..n-1: latitudes half..n-1 about
    // `length` (two loops in the original)
    for (UInt32 row = 0; row < segments; ++row) {
        UInt32 step = row < half ? row + 1 : row;
        Real32 latitude = (Real32)step * latitudeStep;
        Real32 x = radius * Cos(latitude);
        Real32 s = Sin(latitude);
        Vector3* vertex = vertices + 1 + row * stride;
        for (UInt32 j = 0; j <= segments; ++j) {
            Real32 longitude = (Real32)j * longitudeStep;
            vertex[j] = Vector3(row < half ? -x : length - x, Cos(longitude) * radius * s,
                                s * radius * Sin(longitude)) * transform;
        }
    }
    vertices[last] = Vector3(radius + length, 0, 0) * transform;
    const Vector3 first = transform.GetTranslation();
    const Vector3 second = Vector3(length, 0, 0) * transform;

    AddCap(vertices[0], vertices + 1, segments, true, first, radius, line, triangle, info, solid);
    for (UInt32 r = 0; r + 1 < half; ++r)
        AddBand(vertices + 1 + r * stride, vertices + 1 + (r + 1) * stride, segments, first, first, radius,
                line, triangle, info, solid);
    AddBand(vertices + 1 + (half - 1) * stride, vertices + 1 + half * stride, segments, first, second, radius,
            line, triangle, info, solid);
    for (UInt32 r = half; r + 1 < segments; ++r)
        AddBand(vertices + 1 + r * stride, vertices + 1 + (r + 1) * stride, segments, second, second, radius,
                line, triangle, info, solid);
    AddCap(vertices[last], vertices + 1 + (segments - 1) * stride, segments, false, second, radius, line,
           triangle, info, solid);
    delete[] vertices;
}

// Line from `from` to `to` (field space) in `color`.
void AddFieldLine(SimplePhysicsSceneDebugInfo* info, PhysicsSceneDebugInfo::Line& line, const Matrix3D& transform,
                  const Vector3& from, const Vector3& to, const Color& color)
{
    line.m_from = from * transform;
    line.m_to = to * transform;
    line.m_color = color;
    info->m_lines.PushBack(line);
}

// The axes of a field: x red, y green, z blue; magenta / yellow / cyan when the field is
// occupied.
void AddFieldAxes(SimplePhysicsSceneDebugInfo* info, PhysicsSceneDebugInfo::Line& line, const Matrix3D& transform,
                  const ConnectionField& field, Real32 x, Real32 y, Real32 z)
{
    const Vector3 origin(0, 0, 0);
    AddFieldLine(info, line, transform, origin, Vector3(x, 0, 0), field.IsOccupied() ? Color::Magenta() : Color::Red());
    AddFieldLine(info, line, transform, origin, Vector3(0, y, 0), field.IsOccupied() ? Color::Yellow() : Color::Green());
    if (z != 0)
        AddFieldLine(info, line, transform, origin, Vector3(0, 0, z), field.IsOccupied() ? Color::Cyan() : Color::Blue());
}

// One connection field (the field's frame in `elementTransform`). Planar: the grid outline
// (black) and a cross per point (red: full, green: free, faint: type 29); hinge: a cross and
// the axis (-y when reversed) and -x; axle / slider / rail / ball / fixed: the axes (fixed with
// one axis: a circle of radius 0.4 about y). Mac inline in 0xb8f80 (jump table 0xba2fe),
// Win 0x4857f0 (ECX element transform, EDX field)
void AddField(const ConnectionField& field, const Matrix3D& elementTransform, SimplePhysicsSceneDebugInfo* info)
{
    // NOTE: the element transform first, then the field frame (a * b applies b first).
    Matrix3D transform(elementTransform, field.GetTransform());
    PhysicsSceneDebugInfo::Line line;
    switch (field.GetType()) {
    case ConnectionField::TYPE_PLANAR: {
        const ConnectionFieldPlanar& planar = static_cast<const ConnectionFieldPlanar&>(field);
        const UInt32 width = planar.GetWidth();
        const UInt32 height = planar.GetHeight();
        line.m_color = Color::Black();
        const Vector3 origin = Vector3(0, 0, 0) * transform;
        const Vector3 a = Vector3((Real32)width * 0.4f, 0, 0) * transform;
        const Vector3 b = Vector3((Real32)width * 0.4f, 0, (Real32)height * 0.4f) * transform;
        const Vector3 c = Vector3(0, 0, (Real32)height * 0.4f) * transform;
        const Vector3 up = Vector3(0, 1, 0) * transform;
        line.m_from = origin;
        line.m_to = a;
        info->m_lines.PushBack(line);
        line.m_from = a;
        line.m_to = b;
        info->m_lines.PushBack(line);
        line.m_from = b;
        line.m_to = c;
        info->m_lines.PushBack(line);
        line.m_from = c;
        line.m_to = origin;
        info->m_lines.PushBack(line);
        line.m_from = origin;
        line.m_to = up;
        info->m_lines.PushBack(line);
        for (UInt32 row = 0; row <= height; ++row) {
            Real32 z = (Real32)row * 0.4f;
            for (UInt32 column = 0; column <= width; ++column) {
                Real32 x = (Real32)column * 0.4f;
                const ConnectionPoint& point = planar.GetPoint((width + 1) * row + column);
                Color color;
                if (point.m_type == 29)
                    color = Color(0, 0, 0, 0.125f);
                else
                    color = point.m_free == 0 && point.m_capacity != 0 ? Color::Red() : Color::Green();
                AddFieldLine(info, line, transform, Vector3(x - kMarkSize, 0, z - kMarkSize),
                             Vector3(x + kMarkSize, 0, z + kMarkSize), color);
                AddFieldLine(info, line, transform, Vector3(x - kMarkSize, 0, z + kMarkSize),
                             Vector3(x + kMarkSize, 0, z - kMarkSize), color);
            }
        }
        break;
    }
    case ConnectionField::TYPE_HINGE: {
        const ConnectionFieldHinge& hinge = static_cast<const ConnectionFieldHinge&>(field);
        const Color color = hinge.IsOccupied() ? Color::Yellow() : Color::Green();
        AddFieldLine(info, line, transform, Vector3(-kMarkSize, 0, -kMarkSize), Vector3(kMarkSize, 0, kMarkSize), color);
        AddFieldLine(info, line, transform, Vector3(-kMarkSize, 0, kMarkSize), Vector3(kMarkSize, 0, -kMarkSize), color);
        AddFieldLine(info, line, transform, Vector3(0, 0, 0), Vector3(0, hinge.IsReversed() ? -1.0f : 1.0f, 0), color);
        AddFieldLine(info, line, transform, Vector3(0, 0, 0), Vector3(-1, 0, 0), color);
        break;
    }
    case ConnectionField::TYPE_AXLE:
    case ConnectionField::TYPE_SLIDER:
        AddFieldAxes(info, line, transform, field, kMarkSize,
                     static_cast<const ConnectionFieldLineHelper&>(field).GetLength(), kMarkSize);
        break;
    case ConnectionField::TYPE_BALL:
        AddFieldAxes(info, line, transform, field, 0.8f, 0.8f, 0.8f);
        break;
    case ConnectionField::TYPE_FIXED: {
        AddFieldAxes(info, line, transform, field, 0.8f, 0.8f, 0);
        if (static_cast<const ConnectionFieldFixed&>(field).GetAxes() != 1)
            break;
        line.m_color = Color::Green();
        const Real32 step = kTwoPi / 24;
        Real32 s, c;
        SinCos((Real32)0 * step, s, c);
        line.m_to = Vector3(c * 0.4f, 0, s * 0.4f) * transform;
        // NOTE: 25 segments from angle 0 (the first one has zero length)
        for (UInt32 i = 0; i < 25; ++i) {
            line.m_from = line.m_to;
            Real32 angle = (Real32)i * step;
            Real32 sine = Sin(angle);
            Real32 cosine = Cos(angle);
            line.m_to = Vector3(cosine * 0.4f, 0, sine * 0.4f) * transform;
            info->m_lines.PushBack(line);
        }
        break;
    }
    case ConnectionField::TYPE_RAIL:
        AddFieldAxes(info, line, transform, field, kMarkSize, kMarkSize,
                     -Max(static_cast<const ConnectionFieldLineHelper&>(field).GetLength(), kMarkSize));
        break;
    default:
        break;
    }
}

} // namespace

// ---------------------------------------------------------------------------------------------
// InternalGetDebugInfoCollisionVolumeVisitor
// RTTI: LEGO::InternalGetDebugInfoCollisionVolumeVisitor : PrimitiveElement::CollisionVolumeVisitor.
// 0x14 bytes: vptr, colour (NULL: random per volume), margin (added to the sizes / radii),
// solid, the debug info. (name from RTTI; member names inferred)
// ---------------------------------------------------------------------------------------------
class InternalGetDebugInfoCollisionVolumeVisitor : public PrimitiveElement::CollisionVolumeVisitor {
public:
    InternalGetDebugInfoCollisionVolumeVisitor(const Color* color, Real32 margin, bool solid,
                                               SimplePhysicsSceneDebugInfo* info)
        : m_color(color), m_margin(margin), m_solid(solid), m_info(info) {}

    // Mac 0xca080 (slot 0), Win 0x48d600 (slot 2)
    virtual void Visit(PhysicsGeometrySphere* sphere, const Matrix3D& transform)
    {
        Sphere volume(Vector3(0, 0, 0), sphere->GetRadius() + m_margin);
        AddSphere(volume, transform, m_color, m_info, m_solid);
    }
    // Mac 0xca100 (slot 1), Win 0x48d660 (slot 1)
    virtual void Visit(PhysicsGeometryBox* box, const Matrix3D& transform)
    {
        BoundingBox volume(box->GetHalfSize() + m_margin);
        volume.m_min = -volume.m_max;
        AddBox(volume, transform, m_color, m_info, m_solid);
    }
    // Mac 0xca1a0 (slot 2, the capsule drawing inlined), Win 0x48d710 (slot 0)
    virtual void Visit(PhysicsGeometryCapsule* capsule, const Matrix3D& transform)
    {
        Real32 radius = capsule->GetRadius() + m_margin;
        Real32 length = capsule->GetLength();
        AddCapsule(radius, length, transform, m_color, m_info, m_solid);
    }

    const Color*                  m_color;    // +0x04
    Real32                        m_margin;   // +0x08
    bool                          m_solid;    // +0x0c
    SimplePhysicsSceneDebugInfo*  m_info;     // +0x10
};

// ---------------------------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------------------------

namespace {

// One body at `transform` (its world transform). Mac 0xb8f80 (first argument unused), Win
// 0x48d760
void AddBody(const RigidElement* body, const Matrix3D& transform, UInt32 flags, const BasicArray<Part*>* parts,
             SimplePhysicsSceneDebugInfo* info)
{
    if (flags & SimplePhysicsSceneDebugInfo::FlagBodyBox) {
        Color color(1, 1, 0, 1);
        AddBox(body->m_bounding, transform, &color, info, false);
    }
    if (flags & SimplePhysicsSceneDebugInfo::FlagBodySphere) {
        Color color(1, 1, 0, 1);
        AddSphere(body->m_boundingSphere, transform, &color, info, false);
    }
    for (UInt32 r = 0; r < body->m_elements.GetSize(); ++r) {
        const PrimitiveRigidElement* rigid = body->m_elements[r];
        // NOTE: the rigid element's transform first, then the body's (the reverse of
        // PrimitiveElement::GetTransform); both builds.
        Matrix3D rigidTransform(rigid->m_transform, transform);
        if (flags & SimplePhysicsSceneDebugInfo::FlagRigidBox) {
            Color color(1, 0, 1, 1);
            AddBox(rigid->m_bounding, rigidTransform, &color, info, false);
        }
        if (flags & SimplePhysicsSceneDebugInfo::FlagRigidSphere) {
            Color color(1, 0, 1, 1);
            AddSphere(rigid->m_boundingSphere, rigidTransform, &color, info, false);
        }
        for (UInt32 e = 0; e < rigid->m_elements.GetSize(); ++e) {
            const PrimitiveElement* element = rigid->m_elements[e];
            if (parts != 0) {
                const Part* const* end = parts->End();
                const Part* const* it = parts->Begin();
                while (it != end && *it != element->m_part)
                    ++it;
                if (it == end)
                    continue;
            }
            Matrix3D elementTransform(rigidTransform, element->m_transform);
            if (flags & SimplePhysicsSceneDebugInfo::FlagFields) {
                for (UInt32 f = 0; f < element->m_fields.GetSize(); ++f)
                    AddField(*element->m_fields[f], elementTransform, info);
            }
            if (flags & (SimplePhysicsSceneDebugInfo::FlagCollisionVolumes |
                         SimplePhysicsSceneDebugInfo::FlagVolumeColorPerElement)) {
                Color elementColor;
                const Color* color = 0;
                if (flags & SimplePhysicsSceneDebugInfo::FlagVolumeColorPerElement) {
                    elementColor = Color::Random();
                    color = &elementColor;
                }
                InternalGetDebugInfoCollisionVolumeVisitor visitor(
                    color, (flags & SimplePhysicsSceneDebugInfo::FlagVolumeMargin) ? 0.01f : 0.0f,
                    (flags & SimplePhysicsSceneDebugInfo::FlagVolumeWireframe) == 0, info);
                element->VisitCollisionVolumes(visitor, elementTransform);
            }
            if ((flags & SimplePhysicsSceneDebugInfo::FlagFlexAxes) && element->m_part->m_elements.GetSize() > 1) {
                // NOTE: the element's world transform followed by the rigid element's world
                // transform once more (both builds)
                Matrix3D axes(elementTransform, rigidTransform);
                PhysicsSceneDebugInfo::Line line;
                const Vector3 origin = Vector3(0, 0, 0) * axes;
                line.m_from = origin;
                line.m_to = Vector3(0.5f, 0, 0) * axes;
                line.m_color = Color::Red();
                info->m_lines.PushBack(line);
                line.m_from = origin;
                line.m_to = Vector3(0, 0.5f, 0) * axes;
                line.m_color = Color::Green();
                info->m_lines.PushBack(line);
                line.m_from = origin;
                line.m_to = Vector3(0, 0, 0.5f) * axes;
                line.m_color = Color::Blue();
                info->m_lines.PushBack(line);
            }
            if (flags & SimplePhysicsSceneDebugInfo::FlagElementBox) {
                Color color(0, 1, 1, 1);
                AddBox(element->m_primitive->GetBones()[element->m_boneIndex].m_bounding, elementTransform, &color,
                       info, false);
            }
            if (flags & SimplePhysicsSceneDebugInfo::FlagElementSphere) {
                Color color(0, 1, 1, 1);
                AddSphere(element->m_boundingSphere, elementTransform, &color, info, false);
            }
        }
    }
}

} // namespace

// Mac 0xb8ec0, Win 0x48a3e0
PhysicsSceneDebugInfo* Scene::GetDebugInfo(const RigidElement* body, UInt32 flags, const BasicArray<Part*>* parts,
                                           const Matrix3D& transform) const
{
    SimplePhysicsSceneDebugInfo* info = new SimplePhysicsSceneDebugInfo;
    AddBody(body, transform, flags, parts, info);
    return info;
}

// Mac 0xbaa40, Win 0x48a460
PhysicsSceneDebugInfo* Scene::GetDebugInfo(UInt32 flags, const Matrix3D& transform) const
{
    SimplePhysicsSceneDebugInfo* info = new SimplePhysicsSceneDebugInfo;
    srand(1);
    for (UInt32 h = 0; h < m_hingedElements.GetSize(); ++h) {
        const HingedElement* hinged = m_hingedElements[h];
        for (UInt32 b = 0; b < hinged->m_elements.GetSize(); ++b) {
            const RigidElement* body = hinged->m_elements[b];
            Matrix3D bodyTransform;
            body->m_body->GetTransform(bodyTransform);
            Matrix3D world(transform, bodyTransform);
            AddBody(body, world, flags, 0, info);
        }
    }
    return info;
}

// Mac 0xbab80 (Win inlined)
void Scene::SetListener(SceneListener* listener)
{
    SceneListener* old = m_listener;
    if (old == listener)
        return;
    m_listener = listener;
    if (listener != 0)
        listener->AddRef();
    if (old != 0)
        old->Release();
}

} // namespace LEGO
